#![deny(clippy::all)]
#![warn(clippy::pedantic)]

extern crate libc;

use std::ffi::{CStr, CString};
use std::os::raw::{c_char};
use std::{mem::{self, MaybeUninit}, ptr};
use std::sync::{RwLock, Weak, Arc, RwLockReadGuard, RwLockWriteGuard};
use webthing::{
    Action, BaseAction, Event, BaseEvent, BaseProperty, Property, property::ValueForwarder, BaseThing, Thing, ThingsType, WebThingServer, server::ActionGenerator,
};
use actix::prelude::*;
use uuid::Uuid;

// Macros

macro_rules! cstr_to_str {
    ( $v:expr ) => {
        unsafe { CStr::from_ptr($v) }
            .to_str().unwrap().to_owned();
    };
}

macro_rules! cstr_to_json {
    ( $v:expr ) => {
        serde_json::from_str(&cstr_to_str!($v)).unwrap()
    };
}

macro_rules! str_to_cstr {
    ( $v:expr ) => {
        CString::new($v).unwrap().into_raw()
    };
}

macro_rules! json_to_cstr {
    ( $v:expr ) => {
        str_to_cstr!(serde_json::to_string($v).unwrap())
    };
}

macro_rules! from_dbox {
    ( $v:expr, $t:tt ) => {
        {
            #[allow(invalid_value)]
            let zerov: Box<dyn $t> = unsafe { MaybeUninit::<Box<dyn $t>>::zeroed().assume_init() };
            let cpy: *mut Box<dyn $t> = to_box!(zerov);
            unsafe { ptr::copy_nonoverlapping($v, cpy, 1); }
            let res = unsafe{ Box::from_raw(cpy) };
            *res
        }
    };
}

macro_rules! undbox {
    ( $v:expr, | $i:ident : $t:tt | $f:expr ) => {
        {
            let $i = from_dbox!($v, $t);
            let res = {
                $f
            };
            mem::forget($i);
            res
        }
    };
    ( $v:expr, | mut $i:ident : $t:tt | $f:expr ) => {
        {
            let mut $i = from_dbox!($v, $t);
            let res = {
                $f
            };
            mem::forget($i);
            res
        }
    };
    ( | $i:ident : $t:tt | $f:expr ) => {
        undbox!($i, | $i : $t | $f)
    };
    ( | mut $i:ident : $t:tt | $f:expr ) => {
        undbox!($i, | mut $i : $t | $f)
    };
}

macro_rules! to_box {
    ( $v:expr ) => {
        Box::into_raw(Box::new($v))
    };
}

macro_rules! to_dbox {
    ( $v:expr, $t:tt ) => {
        to_box!(Box::new($v) as Box<dyn $t>)
    };
}

macro_rules! str_vec_to_webthing_str_arr {
    ( $v:expr ) => {
        {
            let mut v: Vec<*mut c_char> = $v
                .iter()
                .map(|e| str_to_cstr!(e.to_owned()))
                .collect();
            let res = webthing_str_arr {ptr: v.as_mut_ptr(), len: v.len()};
            mem::forget(v);
            res
        }
    };
}

macro_rules! webthing_str_arr_to_str_vec {
    ( $v:expr )  => {
        {
            let ptr = unsafe { (*$v).ptr };
            let len = unsafe { (*$v).len };
            let mut vec: Vec<String> = Vec::new();
            vec.reserve(len);
            for i in 0..len {
                vec.push(cstr_to_str!(*(ptr.offset(i as isize))));
            }
            vec
        }
    };
}

macro_rules! webthing_thing_lock_arr_to_thing_lock_vec {
    ( $v:expr )  => {
        {
            let ptr = unsafe { (*$v).ptr };
            let len = unsafe { (*$v).len };
            let mut vec: Vec<Arc<RwLock<Box<dyn Thing>>>> = Vec::new();
            vec.reserve(len);
            for i in 0..len {
                vec.push(unsafe { Arc::from_raw(*(ptr.offset(i as isize))) });
            }
            vec
        }
    };
}

macro_rules! to_opt {
    ( $m:tt!($v:expr) ) => {
        to_opt!($v, $m!($v))
    };
    ( $v:expr, $f:expr ) => {
        if ptr::null() == $v {
            None
        } else {
            Some($f)
        }
    };
}

macro_rules! from_opt {
    ( $m:tt!($v:expr) ) => {
        from_opt!($v, |x| $m!(x))
    };
    ( $v:expr, |$i:ident| $f:expr ) => {
        match $v {
            Some($i) => $f,
            None => ptr::null(),
        }
    };
}

macro_rules! result_to_cstr {
    ( $v:expr ) => {
        match $v {
            Ok(()) => ptr::null(),
            Err(x) => str_to_cstr!(x),
        }
    };
}

// Structs

#[derive(Debug)]
#[repr(C)]
pub struct webthing_str_arr {
    ptr: *mut *mut c_char,
    len: usize,
}

#[derive(Debug)]
#[repr(C)]
pub struct webthing_thing_lock_arr {
    ptr: *mut *mut RwLock<Box<dyn Thing>>,
    len: usize,
}

#[derive(Debug)]
#[derive(Clone)]
#[repr(C)]
pub struct webthing_value_forwarder {
    set_value: extern "C" fn (*const c_char) -> *mut c_char,
}
impl ValueForwarder for webthing_value_forwarder {
    fn set_value(&mut self, value: serde_json::Value) -> Result<serde_json::Value, &'static str> {
        let value = json_to_cstr!(&value);
        let res = (self.set_value)(value);
        if ptr::null() == res {
            Err("Err during set_value")
        } else {
            Ok(cstr_to_json!(res))
        }
    }
}

#[derive(Debug)]
#[derive(Clone)]
#[repr(C)]
pub struct webthing_action_generator {
    generate: extern "C" fn (thing: *const RwLock<Box<dyn Thing>>, name: *const c_char, input: *const c_char) -> *mut Box<dyn Action>,
}
impl ActionGenerator for webthing_action_generator {
    fn generate(
        &self,
        thing: Weak<RwLock<Box<dyn Thing>>>,
        name: String,
        input: Option<&serde_json::Value>,
    ) -> Option<Box<dyn Action>> {
        let thing = Arc::into_raw(thing.upgrade().unwrap());
        let name = str_to_cstr!(name);
        let input = from_opt!(json_to_cstr!(input));
        let res = (self.generate)(thing, name, input);
        if ptr::null() == res {
            None
        } else {
            let action = from_dbox!(res, Action);
            Some(action)
        }
    }
}

#[repr(C)]
pub struct webthing_action {
    perform_action: extern "C" fn (thing: *const RwLock<Box<dyn Thing>>, action_name: *const c_char, action_id: *const c_char),
    cancel: Option<extern "C" fn (thing: *const RwLock<Box<dyn Thing>>, action_name: *const c_char, action_id: *const c_char)>,
    _action: BaseAction,
}
impl Action for webthing_action {
    fn set_href_prefix(&mut self, prefix: String) {
        self._action.set_href_prefix(prefix)
    }

    fn get_id(&self) -> String {
        self._action.get_id()
    }

    fn get_name(&self) -> String {
        self._action.get_name()
    }

    fn get_href(&self) -> String {
        self._action.get_href()
    }

    fn get_status(&self) -> String {
        self._action.get_status()
    }

    fn get_time_requested(&self) -> String {
        self._action.get_time_requested()
    }

    fn get_time_completed(&self) -> Option<String> {
        self._action.get_time_completed()
    }

    fn get_input(&self) -> Option<serde_json::Map<String, serde_json::Value>> {
        self._action.get_input()
    }

    fn get_thing(&self) -> Option<Arc<RwLock<Box<dyn Thing>>>> {
        self._action.get_thing()
    }

    fn set_status(&mut self, status: String) {
        self._action.set_status(status)
    }

    fn start(&mut self) {
        self._action.start();
    }

    fn perform_action(&mut self) {
        (self.perform_action)(Arc::into_raw(self.get_thing().unwrap()), str_to_cstr!(self.get_name()), str_to_cstr!(self.get_id()));
    }

    fn cancel(&mut self) {
        match self.cancel {
            None => self._action.cancel(),
            Some(f) => f(Arc::into_raw(self.get_thing().unwrap()), str_to_cstr!(self.get_name()), str_to_cstr!(self.get_id())),
        }
    }

    fn finish(&mut self) {
        self._action.finish();
    }
}

#[derive(Debug)]
#[repr(C)]
pub struct webthing_ssl_options {
    a: *const c_char, 
    b: *const c_char,
}
impl webthing_ssl_options {
    fn convert(self) -> (String, String) {
        let res = (cstr_to_str!(self.a), cstr_to_str!(self.b));
        mem::forget(self);
        res
    }
}

#[repr(C)]
pub struct webthing_thing_read_lock {
    thing: *const Box<dyn Thing>,
    _guard: *const libc::c_void,
}

#[repr(C)]
pub struct webthing_thing_write_lock {
    thing: *const Box<dyn Thing>,
    _guard: *const libc::c_void,
}

#[repr(C)]
pub struct webthing_action_read_lock {
    action: *const Box<dyn Action>,
    _guard: *const libc::c_void,
}

#[repr(C)]
pub struct webthing_action_write_lock {
    action: *const Box<dyn Action>,
    _guard: *const libc::c_void,
}

// Thing functions

#[no_mangle]
pub extern "C" fn webthing_thing_new(
    id: *const c_char,
    title: *const c_char,
    capabilities: *const webthing_str_arr,
    description: *const c_char,
) -> *mut Box<dyn Thing> {
    to_dbox!(BaseThing::new(
        cstr_to_str!(id),
        cstr_to_str!(title),
        to_opt!(webthing_str_arr_to_str_vec!(capabilities)),
        to_opt!(cstr_to_str!(description)),
    ), Thing)
}

#[no_mangle]
pub extern "C" fn webthing_thing_get_id(thing: *mut Box<dyn Thing>) -> *const c_char {
    undbox!( | thing: Thing | {
        str_to_cstr!(thing.get_id())
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_get_title(thing: *mut Box<dyn Thing>) -> *const c_char {
    undbox!( | thing: Thing | {
        str_to_cstr!(thing.get_title())
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_get_description(thing: *mut Box<dyn Thing>) -> *const c_char {
    undbox!( | thing: Thing | {
        str_to_cstr!(thing.get_description())
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_as_thing_description(thing: *mut Box<dyn Thing>) -> *const c_char {
    undbox!( | thing: Thing | {
        json_to_cstr!(&thing.as_thing_description())
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_get_href(thing: *mut Box<dyn Thing>) -> *const c_char {
    undbox!( | thing: Thing | {
        str_to_cstr!(thing.get_href())
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_get_href_prefix(thing: *mut Box<dyn Thing>) -> *const c_char {
    undbox!( | thing: Thing | {
        str_to_cstr!(thing.get_href_prefix())
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_get_ui_href(thing: *mut Box<dyn Thing>) -> *const c_char {
    undbox!( | thing: Thing | {
        let a = thing.get_ui_href();
        from_opt!(str_to_cstr!(a))
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_get_context(thing: *mut Box<dyn Thing>) -> *const c_char {
    undbox!( | thing: Thing | {
        str_to_cstr!(thing.get_context())
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_get_property_descriptions(thing: *mut Box<dyn Thing>) -> *const c_char {
    undbox!( | thing: Thing | {
        json_to_cstr!(&thing.get_property_descriptions())
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_get_action_descriptions(thing: *mut Box<dyn Thing>, action_name: *mut c_char) -> *const c_char {
    undbox!( | thing: Thing | {
        json_to_cstr!(&thing.get_action_descriptions(
            to_opt!(cstr_to_str!(action_name))
        ))
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_get_event_descriptions(thing: *mut Box<dyn Thing>, event_name: *mut c_char) -> *const c_char {
    undbox!( | thing: Thing | {
        json_to_cstr!(&thing.get_event_descriptions(
            to_opt!(cstr_to_str!(event_name))
        ))
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_get_properties(thing: *mut Box<dyn Thing>) -> *const c_char {
    undbox!( | thing: Thing | {
        json_to_cstr!(&thing.get_properties())
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_get_capabilities(thing: *mut Box<dyn Thing>) -> *const webthing_str_arr {
    undbox!( | thing: Thing | {
        to_box!(str_vec_to_webthing_str_arr!(thing.get_type()))
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_get_property(thing: *mut Box<dyn Thing>, property_name: *mut c_char) -> *const c_char {
    undbox!( | thing: Thing | {
        json_to_cstr!(&thing.get_property(&cstr_to_str!(property_name)))
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_add_property(thing: *mut Box<dyn Thing>, property: *mut Box<dyn Property>) {
    undbox!( | mut thing: Thing | {
        thing.add_property(from_dbox!(property, Property));
    });
}

#[no_mangle]
pub extern "C" fn webthing_thing_remove_property(thing: *mut Box<dyn Thing>, property_name: *mut c_char) {
    undbox!( | mut thing: Thing | {
        thing.remove_property(cstr_to_str!(property_name));
    });
}

#[no_mangle]
pub extern "C" fn webthing_thing_set_href_prefix(thing: *mut Box<dyn Thing>, prefix: *mut c_char) {
    undbox!( | mut thing: Thing | {
        thing.set_href_prefix(cstr_to_str!(prefix));
    });
}

#[no_mangle]
pub extern "C" fn webthing_thing_set_ui_href(thing: *mut Box<dyn Thing>, href: *mut c_char) {
    undbox!( | mut thing: Thing | {
        thing.set_ui_href(cstr_to_str!(href));
    });
}

#[no_mangle]
pub extern "C" fn webthing_thing_has_property(thing: *mut Box<dyn Thing>, property_name: *mut c_char) -> bool {
    undbox!( | thing: Thing | {
        thing.has_property(&cstr_to_str!(property_name))
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_set_property(thing: *mut Box<dyn Thing>, property_name: *mut c_char, value: *mut c_char) -> *const c_char {
    undbox!( | mut thing: Thing | {
        result_to_cstr!(thing.set_property(cstr_to_str!(property_name), cstr_to_json!(value)))
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_find_property(thing: *mut Box<dyn Thing>, property_name: *mut c_char) -> *const Box<dyn Property> {
    undbox!( | mut thing: Thing | {
        let property = thing.find_property(&cstr_to_str!(property_name));
        match property {
            None => ptr::null(),
            Some(x) => to_box!(from_dbox!(x, Property)),
        }
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_add_action(thing: *mut Box<dyn Thing>, action: *mut Box<dyn Action>, input: *mut c_char) -> *const c_char {
    undbox!( | mut thing: Thing | {        
        let action = from_dbox!(action, Action);
        let action = Arc::new(RwLock::new(action));
        let input_tmp: serde_json::Value;
        let input = if ptr::null() == input {
            None
        } else {
            input_tmp = cstr_to_json!(input);
            Some(&input_tmp)
        };
        result_to_cstr!(thing.add_action(action, input))
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_remove_action(thing: *mut Box<dyn Thing>, action_name: *mut c_char, action_id: *mut c_char) -> bool {
    undbox!( | mut thing: Thing | {
        thing.remove_action(cstr_to_str!(action_name), cstr_to_str!(action_id))
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_add_available_action(thing: *mut Box<dyn Thing>, name: *mut c_char, metadata: *mut c_char) {
    undbox!( | mut thing: Thing | {
        thing.add_available_action(cstr_to_str!(name), cstr_to_json!(metadata));
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_get_action(thing: *mut Box<dyn Thing>, action_name: *mut c_char, action_id: *mut c_char) -> *const RwLock<Box<dyn Action>> {
    undbox!( | thing: Thing | {
        let action = thing.get_action(cstr_to_str!(action_name), cstr_to_str!(action_id));
        match action {
            None => ptr::null(),
            Some(a) => Arc::into_raw(a)
        }
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_add_event(thing: *mut Box<dyn Thing>, event: *mut Box<dyn Event>) {
    undbox!( | mut thing: Thing | {        
        let event = from_dbox!(event, Event);
        thing.add_event(event)
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_add_available_event(thing: *mut Box<dyn Thing>, name: *mut c_char, metadata: *mut c_char) {
    undbox!( | mut thing: Thing | {
        thing.add_available_event(cstr_to_str!(name), cstr_to_json!(metadata));
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_property_notify(thing: *mut Box<dyn Thing>, name: *mut c_char, value: *mut c_char) {
    undbox!( | mut thing: Thing | {
        thing.property_notify(cstr_to_str!(name), cstr_to_json!(value));
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_action_notify(thing: *mut Box<dyn Thing>, name: *mut c_char) {
    undbox!( | mut thing: Thing | {
        thing.action_notify(cstr_to_json!(name));
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_event_notify(thing: *mut Box<dyn Thing>, name: *mut c_char, event: *mut c_char) {
    undbox!( | mut thing: Thing | {
        thing.event_notify(cstr_to_str!(name), cstr_to_json!(event));
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_start_action(thing: *mut Box<dyn Thing>, name: *mut c_char, id: *mut c_char) {
    undbox!( | mut thing: Thing | {
        thing.start_action(cstr_to_str!(name), cstr_to_str!(id));
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_finish_action(thing: *mut Box<dyn Thing>, name: *mut c_char, id: *mut c_char) {
    undbox!( | mut thing: Thing | {
        thing.finish_action(cstr_to_str!(name), cstr_to_str!(id));
    })
}

#[no_mangle]
pub extern "C" fn webthing_thing_cancel_action(thing: *mut Box<dyn Thing>, name: *mut c_char, id: *mut c_char) {
    undbox!( | mut thing: Thing | {
        thing.cancel_action(cstr_to_str!(name), cstr_to_str!(id));
    })
}

// Action functions

#[no_mangle]
pub extern "C" fn webthing_action_new(
    id: *mut c_char, 
    name: *mut c_char, 
    input: *mut c_char, 
    thing: *mut RwLock<Box<dyn Thing>>, 
    perform_action: extern "C" fn (thing: *const RwLock<Box<dyn Thing>>, action_name: *const c_char, action_id: *const c_char),
    cancel: Option<extern "C" fn (thing: *const RwLock<Box<dyn Thing>>, action_name: *const c_char, action_id: *const c_char)>,
) -> *const Box<dyn Action> {
    let id = if ptr::null() == id {
        Uuid::new_v4().to_string()
    } else {
        cstr_to_str!(id)
    };
    let thingl = unsafe{ Arc::from_raw(thing) };
    let thing = Arc::downgrade(&thingl);
    mem::forget(thingl);
    to_dbox!(webthing_action {
        _action: BaseAction::new(
            id,
            cstr_to_str!(name),
            to_opt!(cstr_to_json!(input)),
            thing,
        ),
        perform_action,
        cancel,
    }, Action)
}

#[no_mangle]
pub extern "C" fn webthing_action_set_href_prefix(action: *mut Box<dyn Action>, prefix: *const c_char) {
    undbox!( | mut action: Action | {
        action.set_href_prefix(cstr_to_str!(prefix));
    })
}

#[no_mangle]
pub extern "C" fn webthing_action_get_id(action: *mut Box<dyn Action>) -> *const c_char {
    undbox!( | action: Action | {
        str_to_cstr!(action.get_id())
    })
}

#[no_mangle]
pub extern "C" fn webthing_action_get_name(action: *mut Box<dyn Action>) -> *const c_char {
    undbox!( | action: Action | {
        str_to_cstr!(action.get_name())
    })
}

#[no_mangle]
pub extern "C" fn webthing_action_get_href(action: *mut Box<dyn Action>) -> *const c_char {
    undbox!( | action: Action | {
        str_to_cstr!(action.get_href())
    })
}

#[no_mangle]
pub extern "C" fn webthing_action_get_status(action: *mut Box<dyn Action>) -> *const c_char {
    undbox!( | action: Action | {
        str_to_cstr!(action.get_status())
    })
}

#[no_mangle]
pub extern "C" fn webthing_action_get_time_requested(action: *mut Box<dyn Action>) -> *const c_char {
    undbox!( | action: Action | {
        str_to_cstr!(action.get_time_requested())
    })
}

#[no_mangle]
pub extern "C" fn webthing_action_get_time_completed(action: *mut Box<dyn Action>) -> *const c_char {
    undbox!( | action: Action | {
        from_opt!(str_to_cstr!(action.get_time_completed()))
    })
}

#[no_mangle]
pub extern "C" fn webthing_action_get_input(action: *mut Box<dyn Action>) -> *const c_char {
    undbox!( | action: Action | {
        from_opt!(json_to_cstr!(&action.get_input()))
    })
}

#[no_mangle]
pub extern "C" fn webthing_action_get_thing(action: *mut Box<dyn Action>) -> *const RwLock<Box<dyn Thing>> {
    undbox!( | action: Action | {
        match action.get_thing() {
            None => ptr::null(),
            Some(t) => Arc::into_raw(t),
        }
    })
}

#[no_mangle]
pub extern "C" fn webthing_action_set_status(action: *mut Box<dyn Action>, status: *mut c_char) {
    undbox!( | mut action: Action | {
        action.set_status(cstr_to_str!(status));
    });
}

#[no_mangle]
pub extern "C" fn webthing_action_start(action: *mut Box<dyn Action>) {
    undbox!( | mut action: Action | {
        action.start();
    })
}

#[no_mangle]
pub extern "C" fn webthing_action_perform(action: *mut Box<dyn Action>) {
    undbox!( | mut action: Action | {
        action.perform_action();
    })
}

#[no_mangle]
pub extern "C" fn webthing_action_finish(action: *mut Box<dyn Action>) {
    undbox!( | mut action: Action | {
        action.finish();
    })
}

#[no_mangle]
pub extern "C" fn webthing_action_cancel(action: *mut Box<dyn Action>) {
    undbox!( | mut action: Action | {
        action.cancel();
    })
}

#[no_mangle]
pub extern "C" fn webthing_action_as_action_description(action: *mut Box<dyn Action>) -> *const c_char {
    undbox!( | action: Action | {
        json_to_cstr!(&action.as_action_description())
    })
}

// Event functions

#[no_mangle]
pub extern "C" fn webthing_event_new(name: *mut c_char, data: *mut c_char) -> *const Box<dyn Event> {
    to_dbox!(BaseEvent::new(
        cstr_to_str!(name),
        cstr_to_json!(data),
    ), Event)
}

#[no_mangle]
pub extern "C" fn webthing_event_get_name(event: *mut Box<dyn Event>) -> *const c_char {
    undbox!( | event: Event | {
        str_to_cstr!(event.get_name())
    })
}

#[no_mangle]
pub extern "C" fn webthing_event_get_data(event: *mut Box<dyn Event>) -> *const c_char {
    undbox!( | event: Event | {
        json_to_cstr!(&event.get_data())
    })
}

#[no_mangle]
pub extern "C" fn webthing_event_get_time(event: *mut Box<dyn Event>) -> *const c_char {
    undbox!( | event: Event | {
        str_to_cstr!(event.get_time())
    })
}

#[no_mangle]
pub extern "C" fn webthing_event_as_event_description(event: *mut Box<dyn Event>) -> *const c_char {
    undbox!( | event: Event | {
        json_to_cstr!(&event.as_event_description())
    })
}

// Property functions

#[no_mangle]
pub extern "C" fn webthing_property_new(name: *mut c_char, initial_value: *mut c_char, value_forwarder: *mut webthing_value_forwarder, metadata: *mut c_char) -> *const Box<dyn Property> {
    let value_forwarder = to_opt!(value_forwarder, {
        let value_forwarder = unsafe { Box::from_raw(value_forwarder) };
        let res = value_forwarder.clone() as Box<dyn ValueForwarder>;
        mem::forget(value_forwarder);
        res
    });
    to_dbox!(BaseProperty::new(
        cstr_to_str!(name),
        cstr_to_json!(initial_value),
        value_forwarder,
        to_opt!(cstr_to_json!(metadata)),
    ), Property)
}

#[no_mangle]
pub extern "C" fn webthing_property_get_name(property: *mut Box<dyn Property>) -> *const c_char {
    undbox!( | property: Property | {
        json_to_cstr!(&property.get_name())
    })
}

#[no_mangle]
pub extern "C" fn webthing_property_get_metadata(property: *mut Box<dyn Property>) -> *const c_char {
    undbox!( | property: Property | {
        json_to_cstr!(&property.get_metadata())
    })
}

#[no_mangle]
pub extern "C" fn webthing_property_get_value(property: *mut Box<dyn Property>) -> *const c_char {
    undbox!( | property: Property | {
        json_to_cstr!(&property.get_value())
    })
}

#[no_mangle]
pub extern "C" fn webthing_property_get_href(property: *mut Box<dyn Property>) -> *const c_char {
    undbox!( | property: Property | {
        str_to_cstr!(property.get_href())
    })
}

#[no_mangle]
pub extern "C" fn webthing_property_set_href_prefix(property: *mut Box<dyn Property>, prefix: *const c_char) {
    undbox!( | mut property: Property | {
        property.set_href_prefix(cstr_to_str!(prefix));
    });
}

#[no_mangle]
pub extern "C" fn webthing_property_set_value(property: *mut Box<dyn Property>, value: *const c_char) -> *const c_char {
    undbox!( | mut property: Property | {
        result_to_cstr!(property.set_value(cstr_to_json!(value)))
    })
}

#[no_mangle]
pub extern "C" fn webthing_property_set_cached_value(property: *mut Box<dyn Property>, value: *const c_char) -> *const c_char {
    undbox!( | mut property: Property | {
        result_to_cstr!(property.set_cached_value(cstr_to_json!(value)))
    })
}

#[no_mangle]
pub extern "C" fn webthing_property_as_property_description(property: *mut Box<dyn Property>) -> *const c_char {
    undbox!( | property: Property | {
        json_to_cstr!(&property.as_property_description())
    })
}

#[no_mangle]
pub extern "C" fn webthing_property_validate_value(property: *mut Box<dyn Property>, value: *const c_char) -> *const c_char {
    undbox!( | property: Property | {
        result_to_cstr!(property.validate_value(&cstr_to_json!(value)))
    })
}

// Server functions

#[no_mangle]
pub extern "C" fn webthing_server_start_single(
    thing: *mut RwLock<Box<dyn Thing>>, 
    port: u16, 
    hostname: *const c_char, 
    ssl_options: *mut webthing_ssl_options, 
    action_generator: *mut webthing_action_generator, 
    base_path: *const c_char, 
    disable_host_validation: bool
) {
    let sys = System::new("");

    let thingl = unsafe { Arc::from_raw(thing) };
    let thing = Arc::clone(&thingl);
    mem::forget(thingl);
    let port = if port == 0 { 
        None
    } else {
        Some(port)
    };
    let ssl_options = to_opt!(ssl_options, unsafe { Box::from_raw(ssl_options) }).map(| e | e.convert());
    let c_action_generator = unsafe { Box::from_raw(action_generator) };
    let action_generator = Box::new(webthing_action_generator {generate: c_action_generator.generate});
    mem::forget(c_action_generator);

    let mut server = WebThingServer::new(
        ThingsType::Single(thing),
        port, 
        to_opt!(cstr_to_str!(hostname)),
        ssl_options, 
        action_generator, 
        to_opt!(cstr_to_str!(base_path)),
        Some(disable_host_validation),
    );
    server.start(None);
    sys.run().unwrap();
}

#[no_mangle]
pub extern "C" fn webthing_server_start_multiple(
    things: *mut webthing_thing_lock_arr, 
    name: *const c_char, 
    port: u16, 
    hostname: *const c_char, 
    ssl_options: *mut webthing_ssl_options, 
    action_generator: *mut webthing_action_generator, 
    base_path: *const c_char, 
    disable_host_validation: bool
) {
    let sys = System::new("");
    let thingsl: Vec<Arc<RwLock<Box<dyn Thing>>>> = webthing_thing_lock_arr_to_thing_lock_vec!(things);
    let things: Vec<Arc<RwLock<Box<dyn Thing>>>> = thingsl.into_iter()
        .map(|t| {
            let res = Arc::clone(&t);
            mem::forget(t);
            res
        })
        .collect();
    let port = if port == 0 { 
        None
    } else {
        Some(port)
    };
    let ssl_options = to_opt!(ssl_options, unsafe { Box::from_raw(ssl_options) }).map(| e | e.convert());
    let c_action_generator = unsafe { Box::from_raw(action_generator) };
    let action_generator = Box::new(webthing_action_generator {generate: c_action_generator.generate});
    mem::forget(c_action_generator);

    let mut server = WebThingServer::new(
        ThingsType::Multiple(things, cstr_to_str!(name)),
        port, 
        to_opt!(cstr_to_str!(hostname)),
        ssl_options, 
        action_generator, 
        to_opt!(cstr_to_str!(base_path)),
        Some(disable_host_validation),
    );
    server.start(None);
    sys.run().unwrap();
}

#[no_mangle]
pub extern "C" fn webthing_thing_lock_new(thing: *mut Box<dyn Thing>) -> *const RwLock<Box<dyn Thing>> {
    let thing = from_dbox!(thing, Thing);
    let lock = Arc::new(RwLock::new(thing));
    Arc::into_raw(lock)
}

#[no_mangle]
pub extern "C" fn webthing_thing_lock_clone(thing: *mut RwLock<Box<dyn Thing>>) -> *const RwLock<Box<dyn Thing>> {
    let thing = unsafe { Arc::from_raw(thing) };
    let lock = thing.clone();
    mem::forget(thing);
    Arc::into_raw(lock)
}

#[no_mangle]
pub extern "C" fn webthing_thing_lock_read(thingl: *mut RwLock<Box<dyn Thing>>) -> *const webthing_thing_read_lock {
    let thingl = unsafe { Arc::from_raw(thingl) };
    let guard = thingl.read().unwrap();
    let thing = from_dbox!(&*guard, Thing);
    let res = to_box!(webthing_thing_read_lock{_guard: to_box!(guard) as *const libc::c_void, thing: to_box!(thing)});
    mem::forget(thingl);
    res
}

#[no_mangle]
pub extern "C" fn webthing_thing_unlock_read(lock: *mut webthing_thing_read_lock) {
    let lock = unsafe { Box::from_raw(lock) };
    let guard = lock._guard as *mut RwLockReadGuard<Box<dyn Thing>>;
    let guard = unsafe { Box::from_raw(guard) };
    mem::drop(guard);
}

#[no_mangle]
pub extern "C" fn webthing_thing_lock_write(thingl: *mut RwLock<Box<dyn Thing>>) -> *const webthing_thing_write_lock {
    let thingl = unsafe { Arc::from_raw(thingl) };
    let guard = thingl.write().unwrap();
    let thing = from_dbox!(&*guard, Thing);
    let res = to_box!(webthing_thing_write_lock{_guard: to_box!(guard) as *const libc::c_void, thing: to_box!(thing)});
    mem::forget(thingl);
    res
}

#[no_mangle]
pub extern "C" fn webthing_thing_unlock_write(lock: *mut webthing_thing_write_lock) {
    let lock = unsafe { Box::from_raw(lock) };
    let guard = lock._guard as *mut RwLockWriteGuard<Box<dyn Thing>>;
    let guard = unsafe { Box::from_raw(guard) };
    mem::drop(guard);
}

#[no_mangle]
pub extern "C" fn webthing_action_lock_read(actionl: *mut RwLock<Box<dyn Action>>) -> *const webthing_action_read_lock {
    let actionl = unsafe { Arc::from_raw(actionl) };
    let guard = actionl.read().unwrap();
    let action = from_dbox!(&*guard, Action);
    let res = to_box!(webthing_action_read_lock{_guard: to_box!(guard) as *const libc::c_void, action: to_box!(action)});
    mem::forget(actionl);
    res
}

#[no_mangle]
pub extern "C" fn webthing_action_unlock_read(lock: *mut webthing_action_read_lock) {
    let lock = unsafe { Box::from_raw(lock) };
    let guard = lock._guard as *mut RwLockReadGuard<Box<dyn Action>>;
    let guard = unsafe { Box::from_raw(guard) };
    mem::drop(guard);
}

#[no_mangle]
pub extern "C" fn webthing_action_lock_write(actionl: *mut RwLock<Box<dyn Action>>) -> *const webthing_action_write_lock {
    let actionl = unsafe { Arc::from_raw(actionl) };
    let guard = actionl.write().unwrap();
    let action = from_dbox!(&*guard, Action);
    let res = to_box!(webthing_action_write_lock{_guard: to_box!(guard) as *const libc::c_void, action: to_box!(action)});
    mem::forget(actionl);
    res
}

#[no_mangle]
pub extern "C" fn webthing_action_unlock_write(lock: *mut webthing_action_write_lock) {
    let lock = unsafe { Box::from_raw(lock) };
    let guard = lock._guard as *mut RwLockWriteGuard<Box<dyn Action>>;
    let guard = unsafe { Box::from_raw(guard) };
    mem::drop(guard);
}

// Free functions

#[no_mangle]
pub extern "C" fn webthing_str_free(str: *mut c_char) {
    mem::drop(unsafe { CString::from_raw(str) } );
}

#[no_mangle]
pub extern "C" fn webthing_str_arr_free(arr: *mut webthing_str_arr) {
    let arr = unsafe { Box::from_raw(arr) };
    let ptr = (*arr).ptr;
    let len = (*arr).len;
    let v: Vec<*mut c_char> = unsafe { Vec::from_raw_parts(ptr, len, len) };
    for e in v {
        mem::drop(unsafe { CString::from_raw(e) } );
    }
    mem::drop(arr);
}

#[no_mangle]
pub extern "C" fn webthing_thing_free(thing: *mut Box<dyn Thing>) {
    mem::drop(unsafe { Box::from_raw(thing) });
}

#[no_mangle]
pub extern "C" fn webthing_property_free(property: *mut Box<dyn Property>) {
    mem::drop(unsafe { Box::from_raw(property) });
}

#[no_mangle]
pub extern "C" fn webthing_action_free(action: *mut Box<dyn Action>) {
    mem::drop(unsafe { Box::from_raw(action) });
}

#[no_mangle]
pub extern "C" fn webthing_event_free(event: *mut Box<dyn Event>) {
    mem::drop(unsafe { Box::from_raw(event) });
}

#[no_mangle]
pub extern "C" fn webthing_thing_lock_free(thing: *const RwLock<Box<dyn Thing>>) {
    mem::drop(unsafe { Arc::from_raw(thing) });
}

#[no_mangle]
pub extern "C" fn webthing_action_lock_free(action: *const RwLock<Box<dyn Action>>) {
    mem::drop(unsafe { Arc::from_raw(action) });
}