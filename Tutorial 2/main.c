//
//  main.c
//  Tutorial 2
//
//  Created by Ramin on 2017-02-15.
//  Copyright © 2017 Ramin. All rights reserved.
//

#include <stdio.h>
#include <gst/gst.h>

int main(int argc, const char * argv[]) {
    g_print("%s\n", "Starting Tutorial 2... .");
    
    GstElement *pipeline, *source, *sink, *converter;
    GstBus *bus;
    GstMessage *message;
    GstStateChangeReturn ret;
    
    gst_init(&argc, &argv);
    
    source = gst_element_factory_make("videotestsrc", "video_source");
    sink = gst_element_factory_make("autovideosink", "video_sink");
    converter = gst_element_factory_make("videoconvert", "video_converter");
    pipeline = gst_pipeline_new("pipeline");
    
    if(!pipeline || !source || !sink || !converter){
        g_printerr("one of the elements cannot be made.", NULL);
        return -1;
    }
    
    gst_bin_add_many(GST_BIN(pipeline), source, sink, converter, NULL);

    if (gst_element_link(source, converter) != TRUE){
        g_printerr("%s\n", "source cannot be connected to the converter for some reason.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    if (gst_element_link(converter, sink) != TRUE){
        g_printerr("%s\n", "converter cannot be connected to the sink for some reason.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    g_object_set(source, "pattern", 1, NULL);
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE){
        g_printerr("Unable to play", NULL);
        g_object_unref(pipeline);
        return -1;
    }
    
    g_print("%s\n", "Waiting for message from the pipeline... .");
    bus = gst_element_get_bus(pipeline);
    message = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    
    if (message != NULL){
        GError *err;
        gchar *debug_info;
        
        switch (GST_MESSAGE_TYPE(message)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(message, &err, &debug_info);
                g_printerr("The following error occured on the element %s: %s.\n", GST_OBJECT_NAME(message->src), err->message);
                g_printerr("Deubg info: %s\n", debug_info ? debug_info: "not available.");
                g_clear_error(&err);
                g_free(debug_info);
                break;
            case GST_MESSAGE_EOS:
                g_print("Reached the end of the show!\n");
                break;
            default:
                g_print("Unexpected message received. \n");
                break;
        }
        
        gst_message_unref(message);
    }
    
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
