#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <regex.h>
#include <pthread.h>
#include "libwebthing.h"

char* on_set_value (char* value) {
    printf("On changed to %s\n", value);
    return value;
}

webthing_thing_lock* make_thing() {
    char** capabilities = malloc(2 * sizeof(char*));
    capabilities[0] = "OnOffSwitch";
    capabilities[1] = "Light";
    webthing_str_arr arr = { .ptr = capabilities, .len = 2 };
    webthing_thing* thing = webthing_thing_new("urn:dev:ops:my-lamp-1234", "My Lamp", &arr, "A web connected lamp");
    free(capabilities);

    char* on_description = 
        "{\"@type\": \"OnOffProperty\","
        "\"title\": \"On/Off\","
        "\"type\": \"boolean\","
        "\"description\": \"Whether the lamp is turned on\"}";
    webthing_value_forwarder on_forwarder = {.set_value = on_set_value};
    webthing_property* on_property = webthing_property_new("on", "true", &on_forwarder, on_description);
    webthing_thing_add_property(thing, on_property);

    char* brightness_description = 
        "{\"@type\": \"BrightnessProperty\","
        "\"title\": \"Brightness\","
        "\"type\": \"integer\","
        "\"description\": \"The level of light from 0-100\","
        "\"minimum\": 0,"
        "\"maximum\": 100,"
        "\"unit\": \"percent\"}";
    webthing_property* brightness_property = webthing_property_new("brightness", "50", NULL, brightness_description);
    webthing_thing_add_property(thing, brightness_property);

    char* fade_metadata = 
        "{\"title\": \"Fade\","
        "\"description\": \"Fade the lamp to a given level\","
        "\"input\": {"
        "    \"type\": \"object\","
        "    \"required\": ["
        "        \"brightness\","
        "        \"duration\""
        "    ],"
        "    \"properties\": {"
        "        \"brightness\": {"
        "            \"type\": \"integer\","
        "            \"minimum\": 0,"
        "            \"maximum\": 100"
        "        },"
        "        \"duration\": {"
        "            \"type\": \"integer\","
        "            \"minimum\": 1"
        "        }"
        "    }"
        "}}";

    webthing_thing_add_available_action(thing, "fade", fade_metadata);

    char* overheated_metadata = 
        "{\"description\": \"The lamp has exceeded its safe operating temperature\","
        "\"type\": \"number\","
        "\"unit\": \"degree celsius\"}";
    webthing_thing_add_available_event(thing, "overheated", overheated_metadata);

    return webthing_thing_lock_new(thing);
}

struct perform_t_args {
    webthing_thing_lock* thing;
    char* action_name;
    char* action_id;
};

void perform_t(void* v) {
    struct perform_t_args* args = (struct perform_t_args*) v;
    webthing_thing_lock* thinglock = args->thing;
    webthing_thing_read_lock* thingrlock = webthing_thing_lock_read(thinglock);

    webthing_action_lock* actionlock = webthing_thing_get_action(thingrlock->thing, args->action_name, args->action_id);
    free(args->action_name);
    free(args->action_id);
    free(args);

    webthing_action_read_lock* actionrlock = webthing_action_lock_read(actionlock);
    
    char* input = webthing_action_get_input(actionrlock->action);
    char* name = webthing_action_get_name(actionrlock->action);
    char* id = webthing_action_get_id(actionrlock->action);

    char* regex = "\\{\"brightness\":([0-9]+),\"duration\":([0-9]+)\\}";
    regex_t regexCompiled;
    size_t maxGroups = 3;
    regmatch_t groupArray[maxGroups];
    if (regcomp(&regexCompiled, regex, REG_EXTENDED)) {
        printf("Could not compile regular expression.\n");
        return 1;
    }
    if (regexec(&regexCompiled, input, maxGroups, groupArray, 0) != 0) {
        printf("Regular expression does not match\n");
        return 1;
    }
    regfree(&regexCompiled);

    char inputcpy[strlen(input) + 1];
    strcpy(inputcpy, input);
    inputcpy[groupArray[1].rm_eo] = 0;
    inputcpy[groupArray[2].rm_eo] = 0;
    char* brightness = inputcpy+groupArray[1].rm_so;
    int duration = atoi(inputcpy+groupArray[2].rm_so);
    webthing_str_free(input);
    
    webthing_thing_unlock_read(thingrlock);

    usleep(duration * 1000);
    
    webthing_thing_write_lock* thingwlock = webthing_thing_lock_write(thinglock);
    webthing_thing_set_property(thingwlock->thing, "brightness", brightness);

    webthing_event* event = webthing_event_new("overheated", "102");
    webthing_thing_add_event(thingwlock->thing, event);

    webthing_action_unlock_read(actionrlock);
    webthing_thing_finish_action(thingwlock->thing, name, id);
    
    printf("Finished action %s(%s)\n", name, id);

    webthing_str_free(name);
    webthing_str_free(id);
    webthing_thing_unlock_write(thingwlock);
    webthing_action_lock_free(actionlock);
    webthing_thing_lock_free(thinglock);
}

void perform(webthing_thing_lock* thing, char* action_name, char* action_id) {
    printf("Performing action %s(%s)\n", action_name, action_id);
    pthread_t thread;
    struct perform_t_args* args = (struct perform_t_args*) malloc(sizeof(struct perform_t_args));
    args->thing = thing;
    args->action_name = malloc((strlen(action_name)+1)*sizeof(char));
    strcpy(args->action_name, action_name);
    args->action_id = malloc((strlen(action_id)+1)*sizeof(char));
    strcpy(args->action_id, action_id);
    pthread_create(&thread, NULL, &perform_t, (void*) args);
    webthing_str_free(action_name);
    webthing_str_free(action_id);
}

webthing_action* generate (webthing_thing_lock* thing, char* name, char* input) {
    webthing_action* action;
    if (input == NULL) { 
        printf("Input missing\n");
        action = NULL;
    } else if (strcmp(name, "fade") != 0) {
        printf("Invalid action name\n");
        action = NULL;
    } else {
        printf("Generating action %s (input: %s)\n", name, input);
        action = webthing_action_new(NULL, "fade", input, thing, perform, NULL);
    }
    webthing_thing_lock_free(thing);
    webthing_str_free(name);
    webthing_str_free(input);
    return action;
}

int main (void) {
    webthing_thing_lock* thing = make_thing();

    webthing_action_generator gen = {.generate = generate};

    printf("Server running\n");
    webthing_server_start_single(
        thing, 8888, NULL, NULL, &gen, NULL, false
    );
    printf("Server terminated\n");
    webthing_thing_lock_free(thing);

    return 0;
}