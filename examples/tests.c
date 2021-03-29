#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "libwebthing.h"

webthing_thing* make_thing() {
    char** capabilities = malloc(2 * sizeof(char*));
    capabilities[0] = "OnOffSwitch";
    capabilities[1] = "Light";
    webthing_str_arr arr = { .ptr = capabilities, .len = 2 };
    webthing_thing* thing = webthing_thing_new("urn:dev:ops:my-lamp-1234", "My Lamp", &arr, "A web connected lamp");
    return thing;
}

webthing_thing* make_thing_without() {
    webthing_thing* thing = webthing_thing_new("urn:dev:ops:my-lamp-1234", "My Lamp", 0, 0);
    return thing;
}

bool set_value_feedback = false;

char* number_set_value (char* value) {
    set_value_feedback = true;
    return value;
}

bool action_perform_feedback = false;

void action_perform (webthing_thing_lock* thing, char* action_name, char* action_id) {
    action_perform_feedback = true;
    webthing_thing_lock_free(thing);
    webthing_str_free(action_name);
    webthing_str_free(action_id);
}

bool action_cancel_feedback = false;

void action_cancel (webthing_thing_lock* thing, char* action_name, char* action_id) {
    action_cancel_feedback = true;
    webthing_thing_lock_free(thing);
    webthing_str_free(action_name);
    webthing_str_free(action_id);
}

webthing_property* make_number_property() {
    webthing_value_forwarder gen = {.set_value = number_set_value};
    webthing_property* prop = webthing_property_new("brightness", "50", &gen, "{\"@type\":\"BrightnessProperty\",\"title\":\"Brightness\",\"type\":\"integer\",\"description\":\"The level of light from 0-100\",\"minimum\":0,\"maximum\":100,\"unit\":\"percent\"}");
    return prop;
}

webthing_property* make_readonly_property() {
    webthing_property* prop = webthing_property_new("brightness", "50", 0, "{\"@type\":\"BrightnessProperty\",\"title\":\"Brightness\",\"type\":\"integer\",\"description\":\"The level of light from 0-100\",\"minimum\":0,\"maximum\":100,\"unit\":\"percent\"}");
    return prop;
}

int main (void) {
    int counter = 0;
    {
        webthing_thing* thing = make_thing();
        char* _ = webthing_thing_get_id(thing);
        assert(strcmp(_, "urn:dev:ops:my-lamp-1234") == 0);
        webthing_str_free(_);
        _ = webthing_thing_get_title(thing);
        assert(strcmp(_, "My Lamp") == 0);
        webthing_str_free(_);
        _ = webthing_thing_get_description(thing);
        assert(strcmp(_, "A web connected lamp") == 0);
        webthing_str_free(_);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        char* _ = webthing_thing_get_id(thing);
        assert(strcmp(_, "urn:dev:ops:my-lamp-1234") == 0);
        webthing_str_free(_);
        _ = webthing_thing_get_id(thing);
        assert(strcmp(_, "urn:dev:ops:my-lamp-1234") == 0);
        webthing_str_free(_);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        char* a = webthing_thing_get_id(thing);
        assert(strcmp(a, "urn:dev:ops:my-lamp-1234") == 0);
        char* b = webthing_thing_get_id(thing);
        assert(strcmp(b, "urn:dev:ops:my-lamp-1234") == 0);
        webthing_str_free(a);
        webthing_str_free(b);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_str_arr* arr = webthing_thing_get_capabilities(thing);
        assert(arr->len == 2);
        assert(strcmp(arr->ptr[0], "OnOffSwitch") == 0);
        assert(strcmp(arr->ptr[1], "Light") == 0);
        webthing_str_arr_free(arr);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing_without();
        char* _ = webthing_thing_get_id(thing);
        assert(strcmp(_, "urn:dev:ops:my-lamp-1234") == 0);
        webthing_str_free(_);
        _ = webthing_thing_get_title(thing);
        assert(strcmp(_, "My Lamp") == 0);
        webthing_str_free(_);
        _ = webthing_thing_get_description(thing);
        assert(strcmp(_, "") == 0);
        webthing_str_free(_);
        webthing_str_arr* a = webthing_thing_get_capabilities(thing);
        assert(a->len == 0);
        webthing_str_arr_free(a);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_str_arr* arr = webthing_thing_get_capabilities(thing);
        assert(arr->len == 2);
        assert(strcmp(arr->ptr[0], "OnOffSwitch") == 0);
        assert(strcmp(arr->ptr[1], "Light") == 0);
        webthing_str_arr_free(arr);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        char* _ = webthing_thing_get_href(thing);
        assert(strcmp(_, "/") == 0);
        webthing_str_free(_);
        _ = webthing_thing_get_href_prefix(thing);
        assert(strcmp(_, "") == 0);
        webthing_str_free(_);
        _ = webthing_thing_get_context(thing);
        assert(strcmp(_, "https://iot.mozilla.org/schemas") == 0);
        webthing_str_free(_);
        _ = webthing_thing_get_ui_href(thing);
        assert(_ == 0);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        char* _ = webthing_thing_as_thing_description(thing);
        assert(strcmp(_, "{\"@context\":\"https://iot.mozilla.org/schemas\",\"@type\":[\"OnOffSwitch\",\"Light\"],\"actions\":{},\"description\":\"A web connected lamp\",\"events\":{},\"id\":\"urn:dev:ops:my-lamp-1234\",\"links\":[{\"href\":\"/properties\",\"rel\":\"properties\"},{\"href\":\"/actions\",\"rel\":\"actions\"},{\"href\":\"/events\",\"rel\":\"events\"}],\"properties\":{},\"title\":\"My Lamp\"}") == 0);
        webthing_str_free(_);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_set_href_prefix(thing, "important");
        char* _ = webthing_thing_get_href_prefix(thing);
        assert(strcmp(_, "important") == 0);
        webthing_str_free(_);
        _ = webthing_thing_get_href(thing);
        assert(strcmp(_, "important") == 0);
        webthing_str_free(_);
        webthing_thing_set_ui_href(thing, "127.0.0.1:54821");
        _ = webthing_thing_get_ui_href(thing);
        assert(strcmp(_, "127.0.0.1:54821") == 0);
        webthing_str_free(_);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_property* property = make_number_property();
        char* _ = webthing_property_get_href(property);
        assert(strcmp(_, "/properties/brightness") == 0);
        webthing_str_free(_);
        _ = webthing_property_get_name(property);
        assert(strcmp(_, "\"brightness\"") == 0); // Why though?
        webthing_str_free(_);
        _ = webthing_property_get_metadata(property);
        assert(strcmp(_, "{\"@type\":\"BrightnessProperty\",\"description\":\"The level of light from 0-100\",\"maximum\":100,\"minimum\":0,\"title\":\"Brightness\",\"type\":\"integer\",\"unit\":\"percent\"}") == 0);
        webthing_str_free(_);
        _ = webthing_property_get_value(property);
        assert(strcmp(_, "50") == 0);
        webthing_str_free(_);
        webthing_property_free(property);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_property* property = make_readonly_property();
        char* _ = webthing_property_get_href(property);
        assert(strcmp(_, "/properties/brightness") == 0);
        webthing_str_free(_);
        _ = webthing_property_get_name(property);
        assert(strcmp(_, "\"brightness\"") == 0); // Why though?
        webthing_str_free(_);
        _ = webthing_property_get_metadata(property);
        assert(strcmp(_, "{\"@type\":\"BrightnessProperty\",\"description\":\"The level of light from 0-100\",\"maximum\":100,\"minimum\":0,\"title\":\"Brightness\",\"type\":\"integer\",\"unit\":\"percent\"}") == 0);
        webthing_str_free(_);
        _ = webthing_property_get_value(property);
        assert(strcmp(_, "50") == 0);
        webthing_str_free(_);
        webthing_property_free(property);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_property* property = make_number_property();
        set_value_feedback = false;
        char* _ = webthing_property_set_value(property, "100");
        assert(_ == 0);
        assert(set_value_feedback);
        _ = webthing_property_get_value(property);
        assert(strcmp(_, "100") == 0);
        webthing_str_free(_);
        webthing_property_free(property);
        counter++;
    } // This test leaks memory. Comparing it to the next case (which doesn't) leads me to conclude that this is either a problem with the value_forwarder or a webthing-rust problem
    printf("Test %i successful\n", counter);
    {
        webthing_property* property = make_number_property();
        set_value_feedback = false;
        char* _ = webthing_property_set_cached_value(property, "100");
        assert(_ == 0);
        assert(!set_value_feedback);
        _ = webthing_property_get_value(property);
        assert(strcmp(_, "100") == 0);
        webthing_str_free(_);
        webthing_property_free(property);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_property* property = make_readonly_property();
        char* _ = webthing_property_set_cached_value(property, "100");
        assert(_ == 0);
        _ = webthing_property_get_value(property);
        assert(strcmp(_, "100") == 0);
        webthing_str_free(_);
        _ = webthing_property_set_value(property, "75");
        assert(_ == 0);
        _ = webthing_property_get_value(property);
        assert(strcmp(_, "75") == 0);
        webthing_str_free(_);
        webthing_property_free(property);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_property* property = make_number_property();
        char* _ = webthing_property_as_property_description(property);
        assert(strcmp(_, "{\"@type\":\"BrightnessProperty\",\"description\":\"The level of light from 0-100\",\"links\":[{\"href\":\"/properties/brightness\",\"rel\":\"property\"}],\"maximum\":100,\"minimum\":0,\"title\":\"Brightness\",\"type\":\"integer\",\"unit\":\"percent\"}") == 0);
        webthing_str_free(_);
        webthing_property_free(property);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_property* property = make_number_property();
        char* _ = webthing_property_validate_value(property, "5");
        assert(_ == 0);
        _ = webthing_property_validate_value(property, "{}");
        assert(strcmp(_, "Invalid property value") == 0);
        webthing_str_free(_);
        webthing_property_free(property);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_property* property = make_number_property();
        bool b = webthing_thing_has_property(thing, "brightness");
        assert(!b);
        webthing_thing_add_property(thing, property);
        b = webthing_thing_has_property(thing, "brightness");
        assert(b);
        char* _ = webthing_thing_get_property(thing, "brightness");
        assert(strcmp(_, "50") == 0);
        webthing_str_free(_);
        webthing_thing_remove_property(thing, "brightness");
        b = webthing_thing_has_property(thing, "brightness");
        assert(!b);
        _ = webthing_thing_get_property(thing, "brightness");
        assert(strcmp(_, "null") == 0);
        webthing_str_free(_);
        webthing_thing_free(thing);
        counter++;
    } // This test is another source of memory leaks. The problem seems to be add_property (therefore all other test cases using this function are affected as well).
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_property* property = make_number_property();
        char* _ = webthing_thing_get_property_descriptions(thing);
        assert(strcmp(_, "{}") == 0);
        webthing_str_free(_);
        _ = webthing_thing_get_properties(thing);
        assert(strcmp(_, "{}") == 0);
        webthing_str_free(_);
        webthing_thing_add_property(thing, property);
        _ = webthing_thing_get_property_descriptions(thing);
        assert(strcmp(_, "{\"brightness\":{\"@type\":\"BrightnessProperty\",\"description\":\"The level of light from 0-100\",\"links\":[{\"href\":\"/properties/brightness\",\"rel\":\"property\"}],\"maximum\":100,\"minimum\":0,\"title\":\"Brightness\",\"type\":\"integer\",\"unit\":\"percent\"}}") == 0);
        webthing_str_free(_);
        _ = webthing_thing_get_properties(thing);
        assert(strcmp(_, "{\"brightness\":50}") == 0);
        webthing_str_free(_);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_property* property = make_number_property();
        char* _ = webthing_thing_set_property(thing, "brightness", "75");
        assert(strcmp(_, "Property not found") == 0);
        webthing_str_free(_);
        webthing_thing_add_property(thing, property);
        _ = webthing_thing_set_property(thing, "brightness", "75");
        assert(_ == 0);
        webthing_thing_property_notify(thing, "brightness", "75");
        _ = webthing_thing_get_properties(thing);
        assert(strcmp(_, "{\"brightness\":75}") == 0);
        webthing_str_free(_);
        _ = webthing_thing_set_property(thing, "brightness", "{}");
        assert(strcmp(_, "Invalid property value") == 0);
        webthing_str_free(_);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_property* property = make_number_property();
        webthing_thing_add_property(thing, property);
        webthing_property* returned_property = webthing_thing_find_property(thing, "darkness");
        assert(returned_property == 0);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_property* property = make_number_property();
        webthing_thing_add_property(thing, property);
        webthing_property* returned_property = webthing_thing_find_property(thing, "brightness");
        assert(returned_property != 0);
        char* _ = webthing_property_get_name(property);
        assert(strcmp(_, "\"brightness\"") == 0);
        webthing_str_free(_);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_lock* lock = webthing_thing_lock_new(thing);
        webthing_thing_read_lock* rlock1 = webthing_thing_lock_read(lock);
        webthing_thing_read_lock* rlock2 = webthing_thing_lock_read(lock);
        char* _ = webthing_thing_get_id(rlock1->thing);
        assert(strcmp(_, "urn:dev:ops:my-lamp-1234") == 0);
        webthing_str_free(_);
        _ = webthing_thing_get_id(rlock2->thing);
        assert(strcmp(_, "urn:dev:ops:my-lamp-1234") == 0);
        webthing_str_free(_);
        webthing_thing_unlock_read(rlock1);
        webthing_thing_unlock_read(rlock2);
        webthing_thing_write_lock* wlock = webthing_thing_lock_write(lock);
        _ = webthing_thing_get_id(wlock->thing);
        assert(strcmp(_, "urn:dev:ops:my-lamp-1234") == 0);
        webthing_str_free(_);
        webthing_thing_unlock_write(wlock);
        webthing_thing_lock_free(thing);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_add_available_action(thing, "fadeoff", "{\"title\": \"Fade to Off\",\"description\": \"Fade the lamp to 0% brightness\"}");
        char* _ = webthing_thing_as_thing_description(thing);
        assert(strcmp(_, "{\"@context\":\"https://iot.mozilla.org/schemas\",\"@type\":[\"OnOffSwitch\",\"Light\"],\"actions\":{\"fadeoff\":{\"description\":\"Fade the lamp to 0% brightness\",\"links\":[{\"href\":\"/actions/fadeoff\",\"rel\":\"action\"}],\"title\":\"Fade to Off\"}},\"description\":\"A web connected lamp\",\"events\":{},\"id\":\"urn:dev:ops:my-lamp-1234\",\"links\":[{\"href\":\"/properties\",\"rel\":\"properties\"},{\"href\":\"/actions\",\"rel\":\"actions\"},{\"href\":\"/events\",\"rel\":\"events\"}],\"properties\":{},\"title\":\"My Lamp\"}") == 0);
        webthing_str_free(_);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_lock* lock = webthing_thing_lock_new(thing);
        webthing_thing_add_available_action(thing, "fadeoff", "{\"title\": \"Fade to Off\",\"description\": \"Fade the lamp to 0% brightness\"}");
        webthing_action* action = webthing_action_new("4353bd33-8e22-4c61-a102-e06113015076", "fadeoff", NULL, lock, action_perform, action_cancel);
        char* _ = webthing_action_get_id(action);
        assert(strcmp(_, "4353bd33-8e22-4c61-a102-e06113015076") == 0);
        webthing_str_free(_);
        _ = webthing_action_get_name(action);
        assert(strcmp(_, "fadeoff") == 0);
        webthing_str_free(_);
        _ = webthing_action_get_href(action);
        assert(strcmp(_, "/actions/fadeoff/4353bd33-8e22-4c61-a102-e06113015076") == 0);
        webthing_str_free(_);
        _ = webthing_action_get_time_requested(action);
        assert(_ != NULL);
        webthing_str_free(_);
        _ = webthing_action_get_input(action);
        assert(_ == NULL);
        webthing_action_free(action);
        webthing_thing_lock_free(lock);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_lock* lock = webthing_thing_lock_new(thing);
        webthing_thing_add_available_action(thing, "fadeoff", "{\"title\": \"Fade to Off\",\"description\": \"Fade the lamp to 0% brightness\"}");
        webthing_action* action = webthing_action_new("4353bd33-8e22-4c61-a102-e06113015076", "fadeoff", NULL, lock, action_perform, action_cancel);
        char* _ = webthing_action_get_status(action);
        assert(strcmp(_, "created") == 0);
        webthing_str_free(_);
        webthing_action_set_status(action, "postponed");
        _ = webthing_action_get_status(action);
        assert(strcmp(_, "postponed") == 0);
        webthing_str_free(_);
        webthing_action_free(action);
        webthing_thing_lock_free(lock);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_lock* lock = webthing_thing_lock_new(thing);
        webthing_thing_add_available_action(thing, "fadeoff", "{\"title\": \"Fade to Off\",\"description\": \"Fade the lamp to 0% brightness\"}");
        webthing_action* action = webthing_action_new("4353bd33-8e22-4c61-a102-e06113015076", "fadeoff", NULL, lock, action_perform, action_cancel);
        char* _ = webthing_action_get_status(action);
        assert(strcmp(_, "created") == 0);
        webthing_str_free(_);
        webthing_action_set_status(action, "postponed");
        _ = webthing_action_get_status(action);
        assert(strcmp(_, "postponed") == 0);
        webthing_str_free(_);
        webthing_action_free(action);
        webthing_thing_lock_free(lock);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_lock* lock = webthing_thing_lock_new(thing);
        webthing_thing_add_available_action(thing, "fadeoff", "{\"title\": \"Fade to Off\",\"description\": \"Fade the lamp to 0% brightness\"}");
        webthing_action* action = webthing_action_new("4353bd33-8e22-4c61-a102-e06113015076", "fadeoff", NULL, lock, action_perform, action_cancel);
        webthing_action_set_href_prefix(action, "prefix");
        char* _ = webthing_action_get_href(action);
        assert(strcmp(_, "prefix/actions/fadeoff/4353bd33-8e22-4c61-a102-e06113015076") == 0);
        webthing_str_free(_);
        webthing_action_free(action);
        webthing_thing_lock_free(lock);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_lock* lock = webthing_thing_lock_new(thing);
        webthing_thing_add_available_action(thing, "fadeoff", "{\"title\": \"Fade to Off\",\"description\": \"Fade the lamp to 0% brightness\"}");
        webthing_action* action = webthing_action_new("4353bd33-8e22-4c61-a102-e06113015076", "fadeoff", NULL, lock, action_perform, action_cancel);
        webthing_thing_add_action(thing, action, NULL);
        webthing_thing_action_notify(thing, "{\"fadeoff\":{\"href\":\"/actions/fadeoff/4353bd33-8e22-4c61-a102-e06113015076\"}}");
        webthing_thing_lock* lock2 = webthing_action_get_thing(action);
        webthing_thing_read_lock* rlock = webthing_thing_lock_read(lock2);
        char* _ = webthing_thing_get_id(rlock->thing);
        assert(strcmp(_, "urn:dev:ops:my-lamp-1234") == 0);
        webthing_str_free(_);
        webthing_thing_unlock_read(rlock);
        webthing_thing_lock_free(lock2);
        webthing_thing_lock_free(lock);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_lock* lock = webthing_thing_lock_new(thing);
        webthing_thing_add_available_action(thing, "fadeoff", "{\"title\": \"Fade to Off\",\"description\": \"Fade the lamp to 0% brightness\"}");
        webthing_action* action = webthing_action_new("4353bd33-8e22-4c61-a102-e06113015076", "fadeoff", NULL, lock, action_perform, action_cancel);
        webthing_thing_add_action(thing, action, NULL);
        webthing_action_lock* lock2 = webthing_thing_get_action(thing, "fadeoff", "4353bd33-8e22-4c61-a102-e06113015076");
        webthing_action_read_lock* rlock = webthing_action_lock_read(lock2);
        char* _ = webthing_action_get_id(rlock->action);
        assert(strcmp(_, "4353bd33-8e22-4c61-a102-e06113015076") == 0);
        webthing_str_free(_);
        webthing_action_unlock_read(rlock);
        webthing_action_lock_free(lock2);
        webthing_thing_remove_action(thing, "fadeoff", "4353bd33-8e22-4c61-a102-e06113015076");
        lock2 = webthing_thing_get_action(thing, "fadeoff", "4353bd33-8e22-4c61-a102-e06113015076");
        assert(lock2 == NULL);
        webthing_thing_lock_free(lock);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_lock* lock = webthing_thing_lock_new(thing);
        webthing_thing_add_available_action(thing, "fadeoff", "{\"title\": \"Fade to Off\",\"description\": \"Fade the lamp to 0% brightness\"}");
        webthing_action* action = webthing_action_new("4353bd33-8e22-4c61-a102-e06113015076", "fadeoff", NULL, lock, action_perform, action_cancel);
        webthing_thing_add_action(thing, action, NULL);
        action_perform_feedback = false;
        webthing_action_start(action);
        char* _ = webthing_action_get_status(action);
        assert(strcmp(_, "pending") == 0);
        webthing_str_free(_);
        assert(!action_perform_feedback);
        webthing_action_perform(action);
        assert(action_perform_feedback);
        action_perform_feedback = false;
        webthing_action_finish(action);
        _ = webthing_action_get_status(action);
        assert(strcmp(_, "completed") == 0);
        webthing_str_free(_);
        assert(!action_perform_feedback);
        _ = webthing_action_get_time_completed(action);
        assert(_ != 0);
        webthing_str_free(_);
        webthing_thing_lock_free(lock);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_lock* lock = webthing_thing_lock_new(thing);
        webthing_thing_add_available_action(thing, "fadeoff", "{\"title\": \"Fade to Off\",\"description\": \"Fade the lamp to 0% brightness\"}");
        webthing_action* action = webthing_action_new("4353bd33-8e22-4c61-a102-e06113015076", "fadeoff", NULL, lock, action_perform, action_cancel);
        webthing_thing_add_action(thing, action, NULL);
        action_cancel_feedback = false;
        webthing_action_start(action);
        assert(!action_cancel_feedback);
        webthing_action_cancel(action);
        assert(action_cancel_feedback);
        webthing_thing_lock_free(lock);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_lock* lock = webthing_thing_lock_new(thing);
        webthing_thing_add_available_action(thing, "fadeoff", "{\"title\": \"Fade to Off\",\"description\": \"Fade the lamp to 0% brightness\"}");
        webthing_action* action = webthing_action_new("4353bd33-8e22-4c61-a102-e06113015076", "fadeoff", NULL, lock, action_perform, action_cancel);
        char* _ = webthing_action_as_action_description(action);
        assert(memcmp("{\"fadeoff\":{\"href\":\"/actions/fadeoff/4353bd33-8e22-4c61-a102-e06113015076\",\"status\":\"created\",\"timeRequested\":\"", _, 111) == 0);
        webthing_str_free(_);
        webthing_action_free(action);
        webthing_thing_lock_free(lock);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_lock* lock = webthing_thing_lock_new(thing);
        webthing_thing_add_available_action(thing, "fadeoff", "{\"title\": \"Fade to Off\",\"description\": \"Fade the lamp to 0% brightness\"}");
        webthing_action* action = webthing_action_new("4353bd33-8e22-4c61-a102-e06113015076", "fadeoff", NULL, lock, action_perform, action_cancel);
        webthing_thing_add_action(thing, action, NULL);
        char* _ = webthing_thing_get_action_descriptions(thing, "fadeoff");
        assert(memcmp("[{\"fadeoff\":{\"href\":\"/actions/fadeoff/4353bd33-8e22-4c61-a102-e06113015076\",\"status\":\"created\",\"timeRequested\":\"", _, 112) == 0);
        webthing_str_free(_);
        webthing_thing_lock_free(lock);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_lock* lock = webthing_thing_lock_new(thing);
        webthing_thing_add_available_action(thing, "fadeoff", "{\"title\": \"Fade to Off\",\"description\": \"Fade the lamp to 0% brightness\"}");
        webthing_action* action = webthing_action_new("4353bd33-8e22-4c61-a102-e06113015076", "fadeoff", NULL, lock, action_perform, action_cancel);
        webthing_thing_add_action(thing, action, NULL);
        webthing_thing_start_action(thing, "fadeoff", "4353bd33-8e22-4c61-a102-e06113015076");
        webthing_thing_finish_action(thing, "fadeoff", "4353bd33-8e22-4c61-a102-e06113015076");
        webthing_thing_lock_free(lock);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_lock* lock = webthing_thing_lock_new(thing);
        webthing_thing_add_available_action(thing, "fadeoff", "{\"title\": \"Fade to Off\",\"description\": \"Fade the lamp to 0% brightness\"}");
        webthing_action* action = webthing_action_new("4353bd33-8e22-4c61-a102-e06113015076", "fadeoff", NULL, lock, action_perform, action_cancel);
        webthing_thing_add_action(thing, action, NULL);
        action_cancel_feedback = false;
        webthing_thing_start_action(thing, "fadeoff", "4353bd33-8e22-4c61-a102-e06113015076");
        assert(!action_cancel_feedback);
        webthing_thing_cancel_action(thing, "fadeoff", "4353bd33-8e22-4c61-a102-e06113015076");
        assert(action_cancel_feedback);
        webthing_thing_lock_free(lock);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_add_available_event(thing, "overheated", "{\"description\":\"too hot\"}");
        char* _ = webthing_thing_as_thing_description(thing);
        assert(strcmp(_, "{\"@context\":\"https://iot.mozilla.org/schemas\",\"@type\":[\"OnOffSwitch\",\"Light\"],\"actions\":{},\"description\":\"A web connected lamp\",\"events\":{\"overheated\":{\"description\":\"too hot\",\"links\":[{\"href\":\"/events/overheated\",\"rel\":\"event\"}]}},\"id\":\"urn:dev:ops:my-lamp-1234\",\"links\":[{\"href\":\"/properties\",\"rel\":\"properties\"},{\"href\":\"/actions\",\"rel\":\"actions\"},{\"href\":\"/events\",\"rel\":\"events\"}],\"properties\":{},\"title\":\"My Lamp\"}") == 0);
        webthing_str_free(_);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_event* event = webthing_event_new("overheated", "123");
        char* _ = webthing_event_get_name(event);
        assert(strcmp(_, "overheated") == 0);
        webthing_str_free(_);
        _ = webthing_event_get_data(event);
        assert(strcmp(_, "123") == 0);
        webthing_str_free(_);
        _ = webthing_event_get_time(event);
        assert(_ != NULL);
        webthing_str_free(_);
        webthing_event_free(event);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_add_available_event(thing, "overheated", "{\"description\":\"too hot\"}");
        webthing_event* event = webthing_event_new("overheated", "123");
        webthing_thing_add_event(thing, event);
        webthing_thing_event_notify(thing, "overheated", "{\"overheated\":{\"data\":123}}");
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);
    {
        webthing_thing* thing = make_thing();
        webthing_thing_add_available_event(thing, "overheated", "{\"description\":\"too hot\"}");
        webthing_event* event = webthing_event_new("overheated", "123");
        webthing_thing_add_event(thing, event);
        char* _ = webthing_event_as_event_description(event);
        assert(memcmp("{\"overheated\":{\"data\":123,\"timestamp\":\"", _, 39) == 0);
        webthing_str_free(_);
        _ = webthing_thing_get_event_descriptions(thing, "overheated");
        assert(memcmp("[{\"overheated\":{\"data\":123,\"timestamp\":\"", _, 40) == 0);
        webthing_str_free(_);
        webthing_thing_free(thing);
        counter++;
    }
    printf("Test %i successful\n", counter);

    printf("\nAll %i tests have passed!\n", counter);

    return 0;
}