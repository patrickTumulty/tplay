#include <gst/gst.h>

#include <iostream>

int main(int argc, char* argv[]) {
    gst_init(&argc, &argv);

    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch("videotestsrc num-buffers=30 ! fakesink", &error);

    if (error != nullptr) {
        std::cerr << "Failed to create pipeline: " << error->message << '\n';
        g_error_free(error);
        return 1;
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* message = gst_bus_timed_pop_filtered(
        bus,
        GST_CLOCK_TIME_NONE,
        static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    int exit_code = 0;
    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
        GError* pipeline_error = nullptr;
        gchar* debug_info = nullptr;
        gst_message_parse_error(message, &pipeline_error, &debug_info);
        std::cerr << "Pipeline error: " << pipeline_error->message << '\n';
        g_clear_error(&pipeline_error);
        g_free(debug_info);
        exit_code = 1;
    }

    gst_message_unref(message);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return exit_code;
}
