extern JSContext* global_js_context;

class JSG {
 public:
  JSG() {
    js_init();
    cx = global_js_context;
  }

  JSContext *cx;
};

extern JSG jsg;
extern JS::Heap<JSObject*> elisp_cons_class_proto;
extern JS::Heap<JSObject*> elisp_string_class_proto;
extern JS::Heap<JSObject*> elisp_symbol_class_proto;
extern JSClass elisp_cons_class;
extern JSClass elisp_string_class;
extern JSClass elisp_symbol_class;
//extern JSClass elisp_marker_class;
//extern JSClass elisp_overlay_class;
//extern JSClass elisp_buffer_class;
//extern JSClass elisp_module_function_class;
extern JS::Heap<JSObject*> elisp_vector_class_proto;
extern JSClass elisp_vector_class;
//extern JSClass elisp_bool_vector_class;
//extern JSClass elisp_char_table_class;
//extern JSClass elisp_sub_char_table_class;
//extern JSClass elisp_subr_class;
//extern JSClass elisp_thread_class;
//extern JSClass elisp_mutex_class;
//extern JSClass elisp_condvar_class;
//extern JSClass elisp_save_value_class;
//extern JSClass elisp_finalizer_class;
//extern JSClass elisp_hash_table_class;
//extern JSClass elisp_frame_class;
//extern JSClass elisp_font_spec_class;
//extern JSClass elisp_font_entity_class;
//extern JSClass elisp_font_object_class;
//extern JSClass elisp_terminal_class;
//extern JSClass elisp_window_class;
//extern JSClass elisp_window_configuration_class;
//extern JSClass elisp_process_class;
//extern JSClass elisp_scroll_bar_class;
//extern JSClass elisp_compiled_class;
//extern JSClass elisp_misc_any_class;
//extern JSClass elisp_vectorlike_class;
