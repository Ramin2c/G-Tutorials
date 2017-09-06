//
//  main.c
//  Overlay
//
//  Created by Ramin on 2017-05-10.
//  Copyright © 2017 Ramin. All rights reserved.
//

#include <stdio.h>
#include <gst/gst.h>
#include <glib.h>
#include <pthread.h>
#include <gst/app/gstappsink.h>
#include <gstdebugutils.h>

GMainLoop *loop;
GstElement *pipeline, *source, *text, *sink, *tee, *appsink, *queue1, *queue2;

int overlay_text(int argc, const char * argv[]);
static GstFlowReturn on_new_sample_from_sink (GstAppSink *sink, gpointer data);
void *textOverlay();


void *textOverlay(){
    //set the text
    int flag = 1;
    g_object_set(text, "color", 6423587, NULL);

    while(1){
        if (flag == 1){
            g_object_set(text, "text", "HELLO LIVELING!\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n", NULL);
        }
        else if (flag == 2){
            g_object_set(text, "text", "HOW ARE YOU?", NULL);
        }
        else if (flag == 3){
            g_object_set(text, "text", "THIS IS A DEMO\n\n\n\n\n\n\n\n\n\n\n\n", NULL);
        }
        else if (flag == 4){
            g_object_set(text, "text", "                                                                                    OF TEXT OVERLAYING", NULL);
        }
        else if (flag == 5){
            g_object_set(text, "text", "IT IS REALLY COOL!\nISN'T IT?", NULL);
            flag = 0;
        }
        g_usleep(1500000);
        flag++;
    }
    return NULL;
}

int overlay_text(int argc, const char * argv[]) {
    
    g_print("%s\n", "Starting Overlay... .");
    
    gst_init(NULL, NULL);
    loop = g_main_loop_new(NULL, FALSE);
    
    pipeline = gst_pipeline_new("player");
    source = gst_element_factory_make("videotestsrc", "video_source");
    text = gst_element_factory_make("textoverlay", "text_overlay");
    sink = gst_element_factory_make("autovideosink", "video_sink");
    tee = gst_element_factory_make("tee", "tee");
    appsink = gst_element_factory_make("appsink", "appsink");
    queue1 = gst_element_factory_make("queue", "que1");
    queue2 = gst_element_factory_make("queue", "que2");
    
    if (!pipeline || !source || !sink || !text || !tee || !appsink){
        g_printerr("one of the elements could not be created.\n");
        return -1;
    }
    
    //g_object_set(source, "location", "/Users/Ramin/LiveLing/blading.wmv", NULL);

    gst_bin_add_many(GST_BIN(pipeline), source, text, tee, queue1, sink, queue2, appsink, NULL); //sink, text, tee, appsink,
    if (!gst_element_link_many(source, text, tee, queue1, sink, NULL)){
        g_printerr("%s\n", "something cannot be connected to something else for some reason!\n");
        gst_object_unref(pipeline);
        return -1;
    }

    if (!gst_element_link_many(tee, queue2, appsink, NULL)){
        g_printerr("%s\n", "something cannot be connected to something else for some reason!\n");
        gst_object_unref(pipeline);
        return -1;
    }

    g_object_set(appsink, "sync", TRUE, NULL);
    g_object_set(appsink, "emit-signals", TRUE, NULL);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample_from_sink), 1);
 
    
    //GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "gst.dot");
    
    //set the video patterm
    g_object_set(source, "pattern", 18, NULL);
                 
    // pthread_t pth;
    // pthread_create(&pth, NULL, textOverlay, "adding the text");
    
    /* Start playing */
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    g_print("%s\n", "Loop started... .");
    g_main_loop_run(loop);
    

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    
    return 0;
}

int cnt = 0;
static GstFlowReturn on_new_sample_from_sink (GstAppSink *sink, gpointer data){
    g_print("on_new_sample_from_sink: %d\n", cnt);
    
    GstSample *sample;
    GstCaps *caps;
    GstBuffer *buf;
    g_signal_emit_by_name (sink, "pull-sample", &sample);
    caps = gst_sample_get_caps (sample);
    buf = gst_sample_get_buffer (sample);
    
    if (sample) {
        gst_sample_unref (sample);
    }
    
    cnt++;
    return GST_FLOW_OK;
}

int main(int argc, const char * argv[]) {
    overlay_text(argc, argv);
}
