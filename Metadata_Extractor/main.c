//
//  main.c
//  Metadata_Extractor
//
//  Created by Ramin on 2017-05-18.
//  Copyright © 2017 Ramin. All rights reserved.
//

#include <stdio.h>
#include <gst/gst.h>
#include <string.h>
#include <gst/app/gstappsink.h>


gboolean initialize(int argc, const char * argv[]);
void createPipeline();
void runMainLoop();
GstFlowReturn on_new_sample_from_sink (GstAppSink *sink, gpointer data);

GMainLoop* loop;
GstElement* pipeline;
GstElement* appsink;
GMainLoop *main_loop;

int main(int argc, const char * argv[]) {
    
    if (!initialize(argc, argv)){
        g_print("GStreamer could not be initialized!\n");
    }
    
    createPipeline(argc, argv);
    if (!pipeline)
        return -1;
    
    runMainLoop();
    
    return 0;
}


gboolean initialize(int argc, const char * argv[]){
    GError* error = NULL;
    gst_init_check(&argc, &argv, &error);
    if(error){
        return FALSE;
    }
    return TRUE;
}
                   
void createPipeline(){
    gchar* strPipeline = "udpsrc uri=udp://localhost:5004 caps=application/x-rtp ! rtpmp2tdepay !  video/mpegts  ! tsdemux name = demux ! queue ! meta/x-klv ! appsink name=appsink demux. ! queue ! fakesink";

    GError* error = NULL;
    pipeline = gst_parse_launch(strPipeline, &error);
    
    if(!pipeline){
        g_print("Pipeline could not be created.\n", NULL);
        return;
    }
    
    if (error){
        g_print("An error occured: %s", error->message);
        return;
    }

    appsink =  gst_bin_get_by_name (GST_BIN (pipeline), "appsink");
    
    if(!appsink){
        g_print("Appsink element could not be created.\n", NULL);
        return;
    }
    
    //connect to new-sample signal
    g_object_set(appsink, "sync", TRUE, NULL);
    g_object_set(appsink, "emit-signals", TRUE, NULL);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample_from_sink), 1);

}

void runMainLoop(){
    GstStateChangeReturn status = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    if (status == GST_STATE_CHANGE_FAILURE){
        g_print("Pipeline could not be started.\n", NULL);
        return;
    }
    
    main_loop = g_main_loop_new (NULL, FALSE);
    if (!main_loop){
        g_print("Mainloop could not be created.\n", NULL);
        return;
    }
    
    g_main_loop_run (main_loop);
}

GstFlowReturn on_new_sample_from_sink (GstAppSink *sink, gpointer data){
    g_print("New sample arrived: ");
    
    GstSample *sample;
    GstCaps *caps;
    GstBuffer *buf;
    g_signal_emit_by_name (sink, "pull-sample", &sample);
    caps = gst_sample_get_caps (sample);
    buf = gst_sample_get_buffer (sample);
    
    if (caps){
        gchar* value = gst_caps_to_string(caps);
        //g_print("%d\n", value);
        g_print(value);
    }
    g_print("\n");

    if(buf){
        g_print(data);
    }

    /*
    if (sample) {
        g_print(sample);
        gst_sample_unref (sample);
    }
    */
    return GST_FLOW_OK;
}








