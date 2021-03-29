#include <stdbool.h>

// Shared structs

/**
 *  @brief A reference representing a thing
 */
typedef struct webthing_thing {} webthing_thing;

/**
 *  @brief A reference representing a thing lock. Use webthing_thing_lock_read or webthing_thing_lock_write to get the associated thing.
 */
typedef struct webthing_thing_lock {} webthing_thing_lock;

/**
 *  @brief A reference representing a property
 */
typedef struct webthing_property {} webthing_property;

/**
 *  @brief A reference representing an action
 */
typedef struct webthing_action {} webthing_action;

/**
 *  @brief A reference representing an action lock. Use webthing_action_lock_read or webthing_action_lock_write to get the associated action.
 */
typedef struct webthing_action_lock {} webthing_action_lock;

/**
 *  @brief A reference representing an event
 */
typedef struct webthing_event {} webthing_event;

/**
 *  @brief A shared array of strings
 */
typedef struct webthing_str_arr {
    char** ptr; /// Pointer to a classical C array of strings
    size_t len; /// Size of the array
} webthing_str_arr;

/**
 *  @brief A shared array of thing locks
 */
typedef struct webthing_thing_lock_arr {
    webthing_thing** ptr; /// Pointer to a classical C array of things
    size_t len; /// Size of the array
} webthing_thing_lock_arr;

/**
 *  @brief A SSL setup
 */
typedef struct webthing_ssl_options {
    char* a;
    char* b;
} webthing_ssl_options;

/**
 *  @brief A value forwarder. Used to handle property changes reported by the gateway.
 */
typedef struct webthing_value_forwarder {
    char* (*set_value) (char* value); /// Gets called whenever a property change was reported by the gateway
} webthing_value_forwarder;

/**
 *  @brief An action generator. Used to handle actions triggered through the gateway.
 */
typedef struct webthing_action_generator {
    webthing_action* (*generate) (webthing_thing_lock* thing, char* name, char* input); /// Gets called whenever an action gets triggered through the gateway
} webthing_action_generator;

/**
 *  @brief A thing locked for read access
 */
typedef struct webthing_thing_read_lock {
    webthing_thing* thing; /// The associated thing
    void* _guard;
} webthing_thing_read_lock;

/**
 *  @brief A thing locked for write access
 */
typedef struct webthing_thing_write_lock {
    webthing_thing* thing; /// The associated thing
    void* _guard;
} webthing_thing_write_lock;

/**
 *  @brief An action locked for read access
 */
typedef struct webthing_action_read_lock {
    webthing_action* action; /// The associated action
    void* _guard;
} webthing_action_read_lock;

/**
 *  @brief An action locked for write access
 */
typedef struct webthing_action_write_lock {
    webthing_action* action; /// The associated action
    void* _guard;
} webthing_action_write_lock;

// Thing functions

/**
* Create a new thing.
*
* @param id the thing's unique ID as string - must be a URI
* @param title the thing's title as string
* @param capabilities the thing's type(s). Set it to null for defaults
* @param description description of the thing as a JSON-encoded string. Set it to null for defaults
* @return pointer to a new thing. Don't forget to call webthing_thing_free!
*/
webthing_thing* webthing_thing_new(char* id, char* title, webthing_str_arr* capabilities, char* description);

/**
* Return the thing state as a Thing Description.
*
* @param thing pointer to the thing
* @return thing state as JSON-encoded string. Don't forget to call webthing_str_free!
*/
char* webthing_thing_as_thing_description(webthing_thing* thing);

/**
* Get the thing's href.
*
* @param thing pointer to the thing
* @return thing's href as string. Don't forget to call webthing_str_free!
*/
char* webthing_thing_get_href(webthing_thing* thing);

/**
* Get the thing's href prefix, i.e. /0.
*
* @param thing pointer to the thing
* @return thing's href prefix as string. Don't forget to call webthing_str_free!
*/
char* webthing_thing_get_href_prefix(webthing_thing* thing);

/**
* Get the UI href.
*
* @param thing pointer to the thing
* @return thing's UI href as string, or null if not set. Don't forget to call webthing_str_free!
*/
char* webthing_thing_get_ui_href(webthing_thing* thing);

/**
* Set the prefix of any hrefs associated with the thing.
*
* @param thing pointer to the thing
* @param perfix prefix as string
*/
void webthing_thing_set_href_prefix(webthing_thing* thing, char* perfix);

/**
* Set the href of this thing's custom UI.
*
* @param thing pointer to the thing
* @param href href as string
*/
void webthing_thing_set_ui_href(webthing_thing* thing, char* href);

/**
* Get the ID of the thing.
*
* @param thing pointer to the thing
* @return thing's ID as string. Don't forget to call webthing_str_free!
*/
char* webthing_thing_get_id(webthing_thing* thing);

/**
* Get the title of the thing.
*
* @param thing pointer to the thing
* @return thing's title as string. Don't forget to call webthing_str_free!
*/
char* webthing_thing_get_title(webthing_thing* thing);

/**
* Get the type context of the thing.
*
* @param thing pointer to the thing
* @return thing's type context as string. Don't forget to call webthing_str_free!
*/
char* webthing_thing_get_context(webthing_thing* thing);

/**
* Get the capability type(s) of the thing.
*
* @param thing pointer to the thing
* @return thing's capabilities as a pointer to a webthing_str_arr. Don't forget to call webthing_str_arr_free!
*/
webthing_str_arr* webthing_thing_get_capabilities(webthing_thing* thing);

/**
* Get the description of the thing.
*
* @param thing pointer to the thing
* @return thing's description as string. Don't forget to call webthing_str_free!
*/
char* webthing_thing_get_description(webthing_thing* thing);

/**
* Get the thing's properties as a JSON map.
*
* @param thing pointer to the thing
* @return thing's properties as a JSON-encoded string. Don't forget to call webthing_str_free!
*/
char* webthing_thing_get_property_descriptions(webthing_thing* thing);

/**
* Get the thing's actions as an array.
*
* @param thing pointer to the thing
* @return thing's actions as a JSON-encoded string. Don't forget to call webthing_str_free!
*/
char* webthing_thing_get_action_descriptions(webthing_thing* thing, char* action_name);

/**
* Get the thing's events as an array.
*
* @param thing pointer to the thing
* @return thing's events as a JSON-encoded string. Don't forget to call webthing_str_free!
*/
char* webthing_thing_get_event_descriptions(webthing_thing* thing, char* event_name);

/**
* Add a property to this thing.
*
* @param thing pointer to the thing
* @param property pointer to the property. The thing will take over ownership of it, so please do not free!
*/
void webthing_thing_add_property(webthing_thing* thing, webthing_property* property);

/**
* Remove a property from this thing.
*
* @param thing pointer to the thing
* @param property_name pointer to the property
*/
void webthing_thing_remove_property(webthing_thing* thing, char* property_name);

/**
* Get a property's value.
*
* @param thing pointer to the thing
* @param property_name name of the property as string
* @return the properties value as a JSON-encoded string, or null if no such propery exists. Don't forget to call webthing_str_free!
*/
char* webthing_thing_get_property(webthing_thing* thing, char* property_name);

/**
* Get a mapping of all properties and their values.
*
* @param thing pointer to the thing
* @return the mapping as a JSON-encoded string. Don't forget to call webthing_str_free!
*/
char* webthing_thing_get_properties(webthing_thing* thing);

/**
* Determine whether or not this thing has a given property.
*
* @param thing pointer to the thing
* @param property_name name of the property as string
* @return whether or not this thing has the given property
*/
bool webthing_thing_has_property(webthing_thing* thing, char* property_name);

/**
* Set a property value.
*
* @param thing pointer to the thing
* @param property_name name of the property as string
* @param value value as JSON-encoded string
* @return null if the operation was successful, or an error message as string otherwise. Don't forget to call webthing_str_free!
*/
char* webthing_thing_set_property(webthing_thing* thing, char* property_name, char* value);

/**
* Set a property value.
*
* @param thing pointer to the thing
* @param property_name name of the property as string
* @return pointer to the desired property, or null if no property with the given name exists.
*/
webthing_property* webthing_thing_find_property(webthing_thing* thing, char* property_name);

/**
* Add a new event and notify subscribers.
*
* @param thing pointer to the thing
* @param event pointer to the event. The thing will take over ownership of it, so please do not free!
*/
void webthing_thing_add_event(webthing_thing* thing, webthing_event* event);

/**
* Add an available event.
*
* @param thing pointer to the thing
* @param name name of the event as string
* @param metadata event metadata, i.e. type, description, etc., as JSON-encoded string
*/
void webthing_thing_add_available_event(webthing_thing* thing, char* name, char* metadata);

/**
* Get an action.
*
* @param thing pointer to the thing
* @param action_name name of the action as string
* @param action_id id of the action as string
* @return pointer to the desired action lock, or null if no action with the given name and id exists. Don't forget to call webthing_action_lock_free!
*/
webthing_action_lock* webthing_thing_get_action(webthing_thing* thing, char* action_name, char* action_id);

/**
* Perform an action on the thing.
*
* @param thing pointer to the thing
* @param action pointer to the action. The thing will take over ownership of it, so please do not free!
* @param input input as JSON-encoded string, or null for no input
* @return null if the operation was successful, or an error message as string otherwise. Don't forget to call webthing_str_free!
*/
char* webthing_thing_add_action(webthing_thing* thing, webthing_action* action, char* input);

/**
* Remove an existing action.
*
* @param thing pointer to the thing
* @param action_name name of the action as string
* @param action_id id of the action as string
* @return a boolean indicating the presence of the action.
*/
bool webthing_thing_remove_action(webthing_thing* thing, char* action_name, char* action_id);

/**
* Add an available action.
*
* @param thing pointer to the thing
* @param name name of the action as string
* @param metadata action metadata, i.e. type, description, etc., as JSON-encoded string
*/
void webthing_thing_add_available_action(webthing_thing* thing, char* name, char* metadata);

/**
* Notify all subscribers of a property change.
*
* @param thing pointer to the thing
* @param name name of the property as string
* @param value value of the property as JSON-encoded string
*/
void webthing_thing_property_notify(webthing_thing* thing, char* name, char* value);

/**
* Notify all subscribers of an action status change.
*
* @param thing pointer to the thing
* @param action action as JSON-encoded string
*/
void webthing_thing_action_notify(webthing_thing* thing, char* action);

/**
* Notify all subscribers of an event.
*
* @param thing pointer to the thing
* @param name name of the event as string
* @param event event as JSON-encoded string
*/
void webthing_thing_event_notify(webthing_thing* thing, char* name, char* event);

/**
* Start the specified action.
*
* @param thing pointer to the thing
* @param name name of the action as string
* @param id id of the action as string
*/
void webthing_thing_start_action(webthing_thing* thing, char* name, char* id);

/**
* Cancel the specified action.
*
* @param thing pointer to the thing
* @param name name of the action as string
* @param id id of the action as string
*/
void webthing_thing_cancel_action(webthing_thing* thing, char* name, char* id);

/**
* Finish the specified action.
*
* @param thing pointer to the thing
* @param name name of the action as string
* @param id id of the action as string
*/
void webthing_thing_finish_action(webthing_thing* thing, char* name, char* id);

// Action functions

/**
* Create a new action.
*
* @param id unique identifier for the action to create as string, or null for defaults
* @param name name of the action as string
* @param input name of the action as string
* @param thing pointer to a thing lock
* @param perform_action pointer to a perform function
* @param cancel pointer to a cancel function, or null for none
* @return pointer to a new action. Don't forget to call webthing_action_free!
*/
webthing_action* webthing_action_new(char* id, char* name, char* input, webthing_thing_lock* thing, void (*perform) (webthing_thing_lock* thing, char* action_name, char* action_id), void (*cancel) (webthing_thing_lock* thing, char* action_name, char* action_id));

/**
* Set the prefix of any hrefs associated with this action.
*
* @param action pointer to the action
* @param perfix prefex as string
*/
void webthing_action_set_href_prefix(webthing_action* action, char* perfix);

/**
* Get the action's ID.
*
* @param action pointer to the action
* @return id of the action as string. Don't forget to call webthing_str_free!
*/
char* webthing_action_get_id(webthing_action* action);

/**
* Get the action's name.
*
* @param action pointer to the action
* @return name of the action as string. Don't forget to call webthing_str_free!
*/
char* webthing_action_get_name(webthing_action* action);

/**
* Get the action's href.
*
* @param action pointer to the action
* @return href of the action as string. Don't forget to call webthing_str_free!
*/
char* webthing_action_get_href(webthing_action* action);

/**
* Get the action's status.
*
* @param action pointer to the action
* @return status of the action as string. Don't forget to call webthing_str_free!
*/
char* webthing_action_get_status(webthing_action* action);

/**
* Get the time the action was requested.
*
* @param action pointer to the action
* @return time the action was requested as string. Don't forget to call webthing_str_free!
*/
char* webthing_action_get_time_requested(webthing_action* action);

/**
* Get the time the action was completed.
*
* @param action pointer to the action
* @return time the action was requested as string, or null if not completed yet. Don't forget to call webthing_str_free!
*/
char* webthing_action_get_time_completed(webthing_action* action);

/**
* Get the inputs for the action.
*
* @param action pointer to the action
* @return inputs of the action as JSON-encoded string, or null if no input is associated with this action. Don't forget to call webthing_str_free!
*/
char* webthing_action_get_input(webthing_action* action);

/**
* Get the thing associated with this action.
*
* @param action pointer to the action
* @return pointer to the associated thing lock, or null if no thing is associated with this action. Don't forget to call webthing_thing_lock_free!
*/
webthing_thing_lock* webthing_action_get_thing(webthing_action* action);

/**
* Set the status of this action.
*
* @param action pointer to the action
* @param status status as string
*/
void webthing_action_set_status(webthing_action* action, char* status);

/**
* Start performing the action.
*
* @param action pointer to the action
*/
void webthing_action_start(webthing_action* action);

/**
* Perform the action.
*
* @param action pointer to the action
*/
void webthing_action_perform(webthing_action* action);

/**
* Cancel performing the action.
*
* @param action pointer to the action
*/
void webthing_action_cancel(webthing_action* action);

/**
* Finish performing the action.
*
* @param action pointer to the action
*/
void webthing_action_finish(webthing_action* action);

/**
* Get the action description.
*
* @param action pointer to the action
* @return action description as JSON-encoded string. Don't forget to call webthing_str_free!
*/
char* webthing_action_as_action_description(webthing_action* action);

// Event functions

/**
* Create a new event.
*
* @param name name of the event as string
* @param data data for the event as JSON-encoded string
* @return pointer to a new event. Don't forget to call webthing_event_free!
*/
webthing_event* webthing_event_new(char* name, char* data);

/**
* Get the event's name.
*
* @param event pointer to the event
* @return name of the event as string. Don't forget to call webthing_str_free!
*/
char* webthing_event_get_name(webthing_event* event);

/**
* Get the event's data.
*
* @param event pointer to the event
* @return data of the event as JSON-encoded string. Don't forget to call webthing_str_free!
*/
char* webthing_event_get_data(webthing_event* event);

/**
* Get the event's timestamp.
*
* @param event pointer to the event
* @return time of the event as string. Don't forget to call webthing_str_free!
*/
char* webthing_event_get_time(webthing_event* event);

/**
* Get the event description.
*
* @param event pointer to the event
* @return event description as JSON-encoded string. Don't forget to call webthing_str_free!
*/
char* webthing_event_as_event_description(webthing_event* event);

// Property functions

/**
* Create a new property.
*
* @param name name of the property as string
* @param initial_value initial property value as JSON.encoded string
* @param value_forwarder value forwarder; property will be read-only if set to null
* @param metadata property metadata, i.e. type, description, unit, etc., as a JSON-encoded string. Set it to null for defaults
* @return pointer to a new property. Don't forget to call webthing_property_free!
*/
webthing_property* webthing_property_new(char* name, char* initial_value, webthing_value_forwarder* value_forwarder, char* metadata);

/**
* Set the prefix of any hrefs associated with this property.
*
* @param property pointer to the property
* @param perfix prefix as string
*/
void webthing_property_set_href_prefix(webthing_property* property, char* perfix);

/**
* Get the href of the property.
*
* @param property pointer to the property
* @return thing's href as string. Don't forget to call webthing_str_free!
*/
char* webthing_property_get_href(webthing_property* property);

/**
* Get the current property value.
*
* @param property pointer to the property
* @return thing's value as JSON-encoded string. Don't forget to call webthing_str_free!
*/
char* webthing_property_get_value(webthing_property* property);

/**
* Set the current value of the property.
*
* @param property pointer to the property
* @param value value as JSON-encoded string
* @return null if the operation was successful, or an error message as string otherwise. Don't forget to call webthing_str_free!
*/
char* webthing_property_set_value(webthing_property* property, char* value);

/**
* Set the cached value of the property.
*
* @param property pointer to the property
* @param value value as JSON-encoded string
* @return null if the operation was successful, or an error message as string otherwise. Don't forget to call webthing_str_free!
*/
char* webthing_property_set_cached_value(webthing_property* property, char* value);

/**
* Get the name of this property.
*
* @param property pointer to the property
* @return property's name as string. Don't forget to call webthing_str_free!
*/
char* webthing_property_get_name(webthing_property* property);

/**
* Get the metadata associated with this property.
*
* @param property pointer to the property
* @return property's metadata as JSON-encoded string. Don't forget to call webthing_str_free!
*/
char* webthing_property_get_metadata(webthing_property* property);

/**
* Get the property description.
*
* @param property pointer to the property
* @return property's description as JSON-encoded string. Don't forget to call webthing_str_free!
*/
char* webthing_property_as_property_description(webthing_property* property);

/**
* Validate new property value before setting it.
*
* @param property pointer to the property
* @param value to test for as JSON-encoded strig
* @return null if the operation was successful, or an error message as string otherwise. Don't forget to call webthing_str_free!
*/
char* webthing_property_validate_value(webthing_property* property, char* value);

// Server functions

/**
* Create a new WebThingServer for a single thing and start listening for incoming connections.
*
* @param thing pointer to the thing lock
* @param port port to listen on. Defaults to 80 if set to 0
* @param hostname optional host name as string, i.e. mything.com, that can be set to null
* @param ssl_options optional pointer to SSL options to pass to the actix web server, that can be set to null
* @param action_generator pointer to action generator struct
* @param base_path base URL to use as string. Defaults to '/' if set to null.
* @param disable_host_validation whether or not to disable host validation. Normally, you will just want to set this to false. Note that disabling host validation can lead to DNS rebinding attacks
*/
void webthing_server_start_single(webthing_thing_lock* thing, unsigned short port, char* hostname, webthing_ssl_options* ssl_options, webthing_action_generator* action_generator, char* base_path, bool disable_host_validation);

/**
* Create a new WebThingServer for a single thing and start listening for incoming connections.
*
* @param things list of things (as locks) managed by this server
* @param name name of this device
* @param port port to listen on. Defaults to 80 if set to 0
* @param hostname optional host name as string, i.e. mything.com, that can be set to null
* @param ssl_options optional pointer to SSL options to pass to the actix web server, that can be set to null
* @param action_generator pointer to action generator struct
* @param base_path base URL to use as string. Defaults to '/' if set to null.
* @param disable_host_validation whether or not to disable host validation. Normally, you will just want to set this to false. Note that disabling host validation can lead to DNS rebinding attacks
*/
void webthing_server_start_multiple(webthing_thing_lock_arr* things, char* name, unsigned short port, char* hostname, webthing_ssl_options* ssl_options, webthing_action_generator* action_generator, char* base_path, bool* disable_host_validation);

/**
* Create a new thing lock
*
* @param thing pointer to the thing. The thing lock will take over ownership of it, so please do not free!
* @return pointer to a lock associated with the given thing. Don't forget to call webthing_thing_lock_free!
*/
webthing_thing_lock* webthing_thing_lock_new(webthing_thing* thing);

/**
* Clone a thing lock
*
* @param thing pointer to the thing lock
* @return pointer to a cloned lock associated with the given thing. Don't forget to call webthing_thing_lock_free!
*/
webthing_thing_lock* webthing_thing_lock_clone(webthing_thing_lock* thing);

/**
* Lock a thing lock for read access
*
* @param thing pointer to the thing lock
* @return pointer to a read lock associated with the given thing lock. Don't forget to call webthing_thing_unlock_read!
*/
webthing_thing_read_lock* webthing_thing_lock_read(webthing_thing_lock* thing);

/**
* Unlock a thing read access
*
* @param lock pointer to the thing read lock
*/
webthing_thing_unlock_read(webthing_thing_read_lock* lock);

/**
* Lock a thing lock for write access
*
* @param thing pointer to the thing lock
* @return pointer to a write lock associated with the given thing lock. Don't forget to call webthing_thing_unlock_write!
*/
webthing_thing_write_lock* webthing_thing_lock_write(webthing_thing_lock* thing);

/**
* Unlock a thing write access
*
* @param lock pointer to the thing write lock
*/
webthing_thing_unlock_write(webthing_thing_write_lock* lock);

/**
* Lock a action lock for read access
*
* @param action pointer to the action lock
* @return pointer to a read lock associated with the given action lock. Don't forget to call webthing_action_unlock_read!
*/
webthing_action_read_lock* webthing_action_lock_read(webthing_action_lock* action);

/**
* Unlock a action read access
*
* @param lock pointer to the action read lock
*/
webthing_action_unlock_read(webthing_action_read_lock* lock);

/**
* Lock a action lock for write access
*
* @param action pointer to the action lock
* @return pointer to a write lock associated with the given action lock. Don't forget to call webthing_action_unlock_write!
*/
webthing_action_write_lock* webthing_action_lock_write(webthing_action_lock* action);

/**
* Unlock a action write access
*
* @param lock pointer to the action write lock
*/
webthing_action_unlock_write(webthing_action_write_lock* lock);


// Free functions

/**
* Free a string that was returned from a webthing function. Only call this method once with every such variable, and never call it with a variable you allocated yourself!
*
* @param str string to free
*/
void webthing_str_free(char* str);

/**
* Free a string array pointer that was returned from a webthing function. Only call this method once with every such variable, and never call it with a variable you allocated yourself!
*
* @param arr pointer to the string array to free
*/
void webthing_str_arr_free(webthing_str_arr* arr);

/**
* Free a thing pointer that was returned from a webthing function. Only call this method once with every such variable, and never call it with a variable you allocated yourself!
*
* @param thing pointer to the thing to free
*/
void webthing_thing_free(webthing_thing* thing);

/**
* Free a property pointer that was returned from a webthing function. Only call this method once with every such variable, and never call it with a variable you allocated yourself!
*
* @param property pointer to the property to free
*/
void webthing_property_free(webthing_property* property);

/**
* Free an action pointer that was returned from a webthing function. Only call this method once with every such variable, and never call it with a variable you allocated yourself!
*
* @param action pointer to the action to free
*/
void webthing_action_free(webthing_action* action);

/**
* Free an event pointer that was returned from a webthing function. Only call this method once with every such variable, and never call it with a variable you allocated yourself!
*
* @param event pointer to the event to free
*/
void webthing_event_free(webthing_event* event);

/**
* Free a thing lock pointer that was returned from a webthing function. Only call this method once with every such variable, and never call it with a variable you allocated yourself!
*
* @param thing pointer to the thing lock to free
*/
void webthing_thing_lock_free(webthing_thing_lock* thing);

/**
* Free an action lock pointer that was returned from a webthing function. Only call this method once with every such variable, and never call it with a variable you allocated yourself!
*
* @param thing pointer to the action lock to free
*/
void webthing_action_lock_free(webthing_action_lock* action);
