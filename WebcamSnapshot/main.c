//
//  main.c
//  Snapshot
//
//  Created by Ramin on 2017-08-29.
//  Copyright © 2017 Ramin. All rights reserved.
//

#include <gst/gst.h>
#include <stdlib.h>
#include <glib.h>
#include <gst/app/gstappsink.h>

#define CAPS "video/x-raw,format=ARGB,width=320,height=240"; //,pixel-aspect-ratio=1/1"

GMainLoop *loop;
GstElement *pipeline, *videoSrc, *capsFilter, *videoConvert, *tee, *queue1, *queue2, *videoSink, *appSink;
GstCaps *caps;

void on_queue_overrun (/* GstQueue *queue */ void *queue, gpointer user_data);
static GstFlowReturn on_new_sample_from_sink (GstAppSink *sink, gpointer data);

int
main (int argc, char *argv[])
{
    
    
    gst_init(&argc, &argv);
    loop = g_main_loop_new(NULL, FALSE);
    
    pipeline = gst_pipeline_new("pipeline");
    videoSrc = gst_element_factory_make("videotestsrc", "source");
    capsFilter = gst_element_factory_make("capsfilter", "capsfilter");
    videoConvert = gst_element_factory_make("videoconvert", "converter");
    tee = gst_element_factory_make("tee", "tee");
    appSink = gst_element_factory_make("appsink", "appsink");
    queue1 = gst_element_factory_make("queue", "q1");
    queue2 = gst_element_factory_make("queue", "q2");
    videoSink = gst_element_factory_make("autovideosink", "videosink");
    caps = gst_caps_from_string("video/x-raw,format=RGB,width=320,height=240");
    g_object_set(G_OBJECT(capsFilter), "caps", caps, NULL);
    
    if(!pipeline || !videoSrc || !tee || !appSink || !videoSink || !queue1 || !queue2){
        g_print("something missing ... ");
        exit(-1);
    }
    
    gst_bin_add_many(GST_BIN(pipeline), videoSrc, tee, queue1, appSink, queue2, videoSink, NULL);
    if(!gst_element_link_many(videoSrc, tee, queue1, videoSink, NULL)){
        g_print("something went wrong when connecting them ... ");
        exit(-1);
    }
    
    if(!gst_element_link_many(tee, queue2, appSink, NULL)){
        g_print("something went wrong when connecting them ... ");
        exit(-1);
    }
    
    g_object_set(videoSrc, "pattern", 13, NULL);
    
    g_object_set(appSink, "emit-signals", TRUE, NULL);
    g_signal_connect(appSink, "new-sample", G_CALLBACK(on_new_sample_from_sink), NULL);
    
    g_object_set(queue1, "silent", TRUE, NULL);
    g_signal_connect(queue1, "overrun", G_CALLBACK(on_queue_overrun), NULL);
    
    g_object_set(queue2, "silent", FALSE, NULL);
    g_signal_connect(queue2, "overrun", G_CALLBACK(on_queue_overrun), NULL);
    guint maxSizeBuffers, maxSizeBytes;
    guint64	maxSizeTime;
    
    short factor = 1;
    g_object_get(queue2, "max-size-buffers", &maxSizeBuffers, "max-size-bytes", &maxSizeBytes, "max-size-time", &maxSizeTime, NULL);
    g_object_set(queue2, "max-size-buffers", maxSizeBuffers * factor, "max-size-bytes", maxSizeBytes * factor, "max-size-time", maxSizeTime * factor * factor, NULL);
    g_object_get(queue2, "max-size-buffers", &maxSizeBuffers, "max-size-bytes", &maxSizeBytes, "max-size-time", &maxSizeTime, NULL);

    /* Start playing */
    GstStateChangeReturn ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
    g_main_loop_run(loop);
    
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
    
    exit (0);
}

void on_queue_overrun (/* GstQueue *queue */ void *queue, gpointer user_data){
    //g_print("queue overrun\n");
}

static GstFlowReturn on_new_sample_from_sink (GstAppSink *sink, gpointer data){
    g_object_set(appSink, "emit-signals", FALSE, NULL);

    GstSample *sample;
    GstCaps *caps;
    GstBuffer *buf;
    
    g_signal_emit_by_name (sink, "pull-sample", &sample);
    
    buf = gst_sample_get_buffer (sample);
    GstMemory* mem = gst_buffer_get_memory(buf, 0);
    GstMapInfo* memMapInfo = (GstMapInfo*)malloc(sizeof(GstMapInfo));
    gst_memory_map(mem, memMapInfo, GST_MAP_READ);
    gsize memSize = memMapInfo->size;

    printf("---------\n");
    for (int i = 0; i < memSize; i+=4) {
        guint8* A = memMapInfo->data + i;
        guint8* R = memMapInfo->data + i + 1;
        guint8* G = memMapInfo->data + i + 2;
        guint8* B = memMapInfo->data + i + 3;

        g_print("color %d %d %d %d\n", *A, *R, *G, *B);
    }
    printf("---------\n");

//    caps = gst_sample_get_caps (sample);
//    buf = gst_sample_get_buffer (sample);
//
//    g_print("on_new_sample_from_sink with duration: %d ns\n", buf->duration);

    if (sample) {
        gst_sample_unref (sample);
    }
    
    return GST_FLOW_OK;

//    g_print("sample received\n");
//    g_object_set(appSink, "emit-signals", FALSE, NULL);
//    printf("---------\n");
//    for (long i = 0; i < 320 * 240 * 4; i++) {
//        char* value = (char*)(data + i);
//        printf("%d\n", *value);
//    }
//    printf("---------\n");
//    return GST_FLOW_OK;
}

