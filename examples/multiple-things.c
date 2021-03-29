#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <regex.h>
#include <pthread.h>
#include "libwebthing.h"

double randomnum() {
    return (double) rand() / (double) RAND_MAX ;
}

char* light_on_set_value (char* value) {
    printf("On changed to %s\n", value);
    return value;
}

void sensor_t (void* arg) {
    webthing_thing_lock* thing = (webthing_thing_lock*) arg;

    while (true) {
        sleep(3);

        float value = (float) abs( 70.0 * randomnum() * (-0.5 + randomnum()) );

        int len = snprintf(NULL, 0, "%f", value);
        char* new_value = (char*) malloc(len + 1);
        snprintf(new_value, len+1, "%f", value);

        printf("setting new humidity level: %s\n", new_value);

        webthing_thing_write_lock* wlock = webthing_thing_lock_write(thing);
        webthing_property* prop = webthing_thing_find_property(wlock->thing, "level");
        char* stat = webthing_property_set_cached_value(prop, new_value);
        if (stat != 0) {
            printf("Failed to update property: %s\n", stat);
            webthing_str_free(stat);
            return 1;
        }
        webthing_thing_property_notify(wlock->thing, "level", new_value);
        
        free(new_value);
        webthing_thing_unlock_write(wlock);
    }

    webthing_thing_lock_free(thing);
}

webthing_thing* make_light() {
    char* capabilities[] = {"OnOffSwitch", "Light"};
    webthing_str_arr arr = { .ptr = capabilities, .len = 2 };
    webthing_thing* thing = webthing_thing_new("urn:dev:ops:my-lamp-1234", "My Lamp", &arr, "A web connected lamp");
    
    char* on_description = 
        "{\"@type\": \"OnOffProperty\","
        "\"title\": \"On/Off\","
        "\"type\": \"boolean\","
        "\"description\": \"Whether the lamp is turned on\"}";
    webthing_value_forwarder on_forwarder = {.set_value = light_on_set_value};
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

webthing_thing* make_sensor() {
    char* capabilities[] = {"MultiLevelSensor"};
    webthing_str_arr arr = { .ptr = capabilities, .len = 1 };
    webthing_thing* thing = webthing_thing_new("urn:dev:ops:my-humidity-sensor-1234", "My Humidity Sensor", &arr, "A web connected humidity sensor");
    
    char* level_description = 
        "{\"@type\": \"LevelProperty\","
        "\"title\": \"Humidity\","
        "\"type\": \"number\","
        "\"description\": \"The current humidity in %\","
        "\"minimum\": 0,"
        "\"maximum\": 100,"
        "\"unit\": \"percent\","
        "\"readOnly\": true}";
    webthing_property* level_property = webthing_property_new("level", "0", NULL, level_description);
    webthing_thing_add_property(thing, level_property);

    webthing_thing_lock* lock = webthing_thing_lock_new(thing);

    pthread_t thread;
    pthread_create(&thread, NULL, &sensor_t, (void*) webthing_thing_lock_clone(lock));

    return lock;
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
    webthing_thing_lock* light = make_light();
    webthing_thing_lock* sensor = make_sensor();

    webthing_thing_lock** thingsptr = malloc(2 * sizeof(webthing_thing*));
    thingsptr[0] = light;
    thingsptr[1] = sensor;
    webthing_thing_lock_arr things = {.ptr=thingsptr, .len=2};
    
    webthing_action_generator gen = {.generate = generate};
    
    printf("Server running\n");
    webthing_server_start_multiple(
        &things, "LightAndTempDevice", 8888, NULL, NULL, &gen, NULL, false
    );
    printf("Server terminated\n");
    
    free(thingsptr);
    webthing_thing_lock_free(light);
    webthing_thing_lock_free(sensor);

    return 0;
}