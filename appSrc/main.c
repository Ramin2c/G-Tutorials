//
//  main.c
//  appSrc
//
//  Created by Ramin on 2017-08-25.
//  Copyright © 2017 Ramin. All rights reserved.
//

// example appsrc for gstreamer 1.0 with own mainloop & external buffers. based on example from gstreamer docs.
// public domain, 2015 by Florian Echtler <floe@butterbrot.org>. compile with:
// gcc --std=c99 -Wall $(pkg-config --cflags gstreamer-1.0) -o gst gst.c $(pkg-config --libs gstreamer-1.0) -lgstapp-1.0

// example appsrc for gstreamer 1.0 with own mainloop & external buffers. based on example from gstreamer docs.
// public domain, 2015 by Florian Echtler <floe@butterbrot.org>. compile with:
// gcc --std=c99 -Wall $(pkg-config --cflags gstreamer-1.0) -o gst gst.c $(pkg-config --libs gstreamer-1.0) -lgstapp-1.0

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <stdint.h>
#include "tcpSource.h"

int want = 1;

uint8_t b_white[384*288*3];
uint8_t b_black[384*288*3];

uint8_t b_color[200*200*4];

int width = 200;
int height = 200;
int planeCount = 4;

static void prepare_buffer(GstAppSrc* appsrc) {
    
    static gboolean white = FALSE;
    static GstClockTime timestamp = 0;
    GstBuffer *buffer;
    guint size;
    GstFlowReturn ret;
    
    if (!want) return;
    want = 0;
    
    size = width * height * planeCount;
    
    buffer = gst_buffer_new_wrapped_full( 0, (gpointer)(white?b_color:b_color), size, 0, size, NULL, NULL );
    
    white = !white;
    
    GST_BUFFER_PTS (buffer) = timestamp;
    GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 1);
    
    timestamp += GST_BUFFER_DURATION (buffer);
    
    ret = gst_app_src_push_buffer(appsrc, buffer);
    
    if (ret != GST_FLOW_OK) {
        /* something wrong, stop pushing */
        // g_main_loop_quit (loop);
    }
}

static void cb_need_data (GstElement *appsrc, guint unused_size, gpointer user_data) {
    //prepare_buffer((GstAppSrc*)appsrc);
    want = 1;
}

gint main (gint argc, gchar *argv[]) {
    
    setupTCPSource(5555);
    
    GstElement *pipeline, *appsrc, *conv, *videosink;
    
//    for (int i = 0; i < 384*288*3; i++) { b_black[i] = 0; b_white[i] = 0xFF; }
    for (int i = 0; i < 200*200*4; i++) { b_color[i] = 0xFF; }
//    int stride = width/(planeCount - 1);
//    for (int i = 0; i < height * planeCount; i++) {
//        //fill the color matrix
//        int index = i * (width);
//
//        //pixels 0 - 127 fill red 0xFF0000
//        for (int j = 0; j < stride; j++) {
//            b_color[index + j] = 0xFF;
//            b_color[index + j + 1] = 0;
//            b_color[index + j + 2] = 0;
//            j += planeCount;
//        }
//        
//        //pixels 128 - 255 fill green 0x00FF00
//        for (int j = 0; j < stride; j++) {
//            b_color[index + stride + j] = 0;
//            b_color[index + stride + j + 1] = 0xFF;
//            b_color[index + stride + j + 2] = 0;
//            j += planeCount;
//        }
//
//        //pixels 256 - 383 fill blue 0x0000FF
//        for (int j = 0; j < stride; j++) {
//            b_color[index + stride * 2 + j] = 0;
//            b_color[index + stride * 2 + j + 1] = 0;
//            b_color[index + stride * 2 + j + 2] = 0xFF;
//            j += planeCount;
//        }
//    }
//    int even = 1;
//    for (int i = 0; i < 385*288; i++) {
//        b_black[i] = 0;
//        if (even)
//            b_white[i] = 0xFFFF;
//        else
//            b_white[i] = 0;
//        
//        even = !even;
//    }
    
    /* init GStreamer */
    gst_init (&argc, &argv);
    
    /* setup pipeline */
    pipeline = gst_pipeline_new ("pipeline");
    appsrc = gst_element_factory_make ("appsrc", "source");
    conv = gst_element_factory_make ("videoconvert", "conv");
    videosink = gst_element_factory_make ("autovideosink", "videosink");
    
    /* setup */
    g_object_set (G_OBJECT (appsrc), "caps",
                  gst_caps_new_simple ("video/x-raw",
                                       "format", G_TYPE_STRING, "ARGB",
                                       "bpp", G_TYPE_INT, 32,
                                       "width", G_TYPE_INT, width,
                                       "height", G_TYPE_INT, height,
                                       "framerate", GST_TYPE_FRACTION, 0, 1,
                                       NULL), NULL);
    gst_bin_add_many (GST_BIN (pipeline), appsrc, conv, videosink, NULL);
    gst_element_link_many (appsrc, conv, videosink, NULL);
    
    /* setup appsrc */
    g_object_set (G_OBJECT (appsrc),
                  "stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
                  "format", GST_FORMAT_TIME,
                  "is-live", TRUE,
                  NULL);
    g_signal_connect (appsrc, "need-data", G_CALLBACK (cb_need_data), NULL);
    
    /* play */
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    
    while (1) {
        prepare_buffer((GstAppSrc*)appsrc);
        g_main_context_iteration(g_main_context_default(),FALSE);
    }
    
    /* clean up */
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (GST_OBJECT (pipeline));
    
    return 0;
}
