
#include <gst/gst.h>
#include <iostream>

int main(int argc, char *argv[])
{
    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create the elements
    GstElement *pipeline = gst_pipeline_new("my-pipeline");
    GstElement *source = gst_element_factory_make("videotestsrc", "source");
    GstElement *sink = gst_element_factory_make("autovideosink", "sink");

    // Check if elements were created successfully
    if (!pipeline || !source || !sink)
    {
        std::cerr << "Not all elements could be created." << std::endl;
        return -1;
    }

    // Build the pipeline by adding elements and linking them
    gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);
    if (gst_element_link(source, sink) != TRUE)
    {
        std::cerr << "Elements could not be linked." << std::endl;
        gst_object_unref(pipeline);
        return -1;
    }

    // Set the pipeline to the PLAYING state
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Wait for 3 seconds to let it run
    std::cout << "Pipeline running..." << std::endl;
    g_usleep(3 * G_USEC_PER_SEC);

    // Tear down and clean up
    std::cout << "Stopping pipeline..." << std::endl;
    gst_element_set_state(pipeline, GST_STATE_NULL);

    // Unreference the pipeline to free all internal elements and memory
    gst_object_unref(pipeline);

    return 0;
}
