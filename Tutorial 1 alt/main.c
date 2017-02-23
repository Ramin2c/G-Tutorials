//
//  main.c
//  Tutorial 1 alt
//
//  Created by Ramin on 2017-02-23.
//  Copyright © 2017 Ramin. All rights reserved.
//

#include <stdio.h>
#include <gst/gst.h>

/*
    playbin flags
    borrowed from playback-tutorial-1.c
*/
typedef enum {
    GST_PLAY_FLAG_VIDEO         = (1 << 0), /* We want video output */
    GST_PLAY_FLAG_AUDIO         = (1 << 1), /* We want audio output */
    GST_PLAY_FLAG_TEXT          = (1 << 2)  /* We want subtitle output */
} GstPlayFlags;

int main(int argc, const char * argv[]){
    GstElement *playbin;
    GstStateChangeReturn ret;
    GMainLoop *main_loop;
    GstBus *bus;
    GstMessage *msg;
    GstPlayFlags flags;
    
    /* Initialize GStreamer */
    gst_init (&argc, &argv);
    
    /* Create the elements */
    playbin = gst_element_factory_make ("playbin", "playbin");
    
    if (!playbin) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }
    
    /* Set the URI to play */
    g_object_set (playbin, "uri", "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm", NULL);
    //https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_cropped_multilingual.webm
    
    /* Set flags to show Audio and Video but ignore Subtitles */
    g_object_get (playbin, "flags", &flags, NULL);
    flags |= GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO; // switch on/off audio and video streams
    g_object_set (playbin, "flags", flags, NULL);
    bus = gst_element_get_bus(playbin);
    
    ret = gst_element_set_state (playbin, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (playbin);
        return -1;
    }
    
    main_loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (main_loop);
    
    msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    /* Free resources */
    if (msg != NULL)
        gst_message_unref (msg);

    /* Free resources */
    g_main_loop_unref (main_loop);
    gst_object_unref (bus);
    gst_element_set_state (playbin, GST_STATE_NULL);
    gst_object_unref (playbin);
    
    return 0;
}
