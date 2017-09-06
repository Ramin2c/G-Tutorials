//
//  tsplayer.c
//  GStreamer Tutorials
//
//  Created by Mudassar on 22/11/16.
//
//
//
//  basic-mux.c
//  GStreamer Tutorials
//
//  Created by Mudassar on 25/10/16.
//
//

//#include "basic-mux.h"
#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <string.h>
#include <gst/app/gstappsrc.h>

#define CHUNK_SIZE 2   /* Amount of bytes we are sending in each buffer */
#define SAMPLE_RATE 2 /* Samples per second we are sending */

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
    GstElement *pipeline, *v_source, *audio_queue, *app_queue ; //*audio_sink,*tee, ;
    GstElement  *video_convert,*video_sink,*video_queue; //, *audio_convert2, *visual,,;
    GstElement *demux, *v_decoder,*a_decoder, *app_sink, *video_parse , *rtpdepay, *filter;
    
    
    
    guint64 num_samples;   /* Number of samples generated so far (for timestamp generation) */
    gfloat a, b, c, d;     /* For waveform generation */
    
    guint sourceid;        /* To control the GSource */
    
    GMainLoop *main_loop;  /* GLib's Main Loop */
} CustomData;

/* The appsink has received a buffer */

static GstFlowReturn new_sample (GstElement *sink, CustomData *data) {
    GstSample *sample;
    
    
    /* Retrieve the buffer */
    g_signal_emit_by_name (sink, "pull-sample", &sample);
    if (sample) {
        /* The only thing we do in this example is print a * to indicate a received buffer*/
        
        
        /* GstSample *sample = gst_app_sink_pull_sample(appsink);*/
        GstBuffer *buffer = gst_sample_get_buffer(sample);
        gint16 *raw;
        GstMapInfo map;
        
        gint num_samples = CHUNK_SIZE / 2; /* Because each sample is 16 bits */
        //gint num_samples = 150;
        gst_buffer_map (buffer, &map, GST_MAP_READ);
        raw = (gint16 *)map.data;
        //THROUGH map.data you can take out the data, after than unmap the buffer.
        
        g_print (" \n");
        for (int i = 0; i < num_samples; i++) {
            
            g_print ("%c ",raw[i]);
            
            
        }
        
        gst_buffer_unmap (buffer, &map);
        
        //g_print ("*-");
        
        gst_sample_unref (sample);
        //gst_buffer_unref (buffer);
        
        // I commented gst_buffer_unref (buffer) to avoid following error message: gst_mini_object_unref: assertion 'mini_object->refcount > 0' failed
        // I got thie hint from http://gstreamer-devel.966125.n4.nabble.com/gst-buffer-unref-and-pad-push-td972152.html
        
        return GST_FLOW_OK;
        
    }
    return GST_FLOW_ERROR;
    
}


/* This function is called when an error message is posted on the bus */
static void error_cb (GstBus *bus, GstMessage *msg, CustomData *data) {
    GError *err;
    gchar *debug_info;
    
    /* Print error details on the screen */
    gst_message_parse_error (msg, &err, &debug_info);
    g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
    g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
    g_clear_error (&err);
    g_free (debug_info);
    
    g_main_loop_quit (data->main_loop);
}

/* Handler for the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *pad, CustomData *data);

int main(int argc, char *argv[]) {
    CustomData data;
    
    GstCaps *filtercaps;
    GstAudioInfo info;
    GstCaps *audio_caps;
    GstBus *bus;
    GstStateChangeReturn retr;
    gboolean terminate = FALSE;
    GstMessage *msg;
    
    /* Initialize cumstom data structure */
    memset (&data, 0, sizeof (data));
    data.b = 1; /* For waveform generation */
    data.d = 1;
    
    /* Initialize GStreamer */
    gst_init (&argc, &argv);
    
    /* Create the elements */
    //data.app_source = gst_element_factory_make ("appsrc", "audio_source");
    //data.tee = gst_element_factory_make ("tee", "tee");
    //data.audio_queue = gst_element_factory_make ("queue", "audio_queue");
    //data.audio_convert = gst_element_factory_make ("audioconvert", "audio_convert");
    //data.audio_resample = gst_element_factory_make ("audioresample", "audio_resample");
    //data.audio_sink = gst_element_factory_make ("autoaudiosink", "audio_sink");
    
    data.video_queue = gst_element_factory_make ("queue", "video_queue");
    
    data.video_convert = gst_element_factory_make ("videoconvert", "video_convert");
    data.video_sink = gst_element_factory_make ("osxvideosink", "video_sink");
    
    data.rtpdepay=gst_element_factory_make ("rtpmp2tdepay", "rtpdepay");
    
    data.app_queue = gst_element_factory_make ("queue", "app_queue");
    data.app_sink = gst_element_factory_make ("appsink", "app_sink");
    data.video_parse =gst_element_factory_make ("h264parse", "video_parse");
    data.demux = gst_element_factory_make("tsdemux", "demux");
    data.filter = gst_element_factory_make ("capsfilter", "filter");
    
    //data.mpeg_parse = gst_element_factory_make("mpegvideoparse", "mpeg_parse");
    
    
    
    data.v_source = gst_element_factory_make ("udpsrc", "av_source");
    data.v_decoder = gst_element_factory_make("avdec_h264", "v_decoder");
    //data.a_decoder = gst_element_factory_make("voaacdec", "a_decoder");
    
    
    
    
    /* Create the empty pipeline */
    data.pipeline = gst_pipeline_new ("test-pipeline");
    
    if (!data.pipeline || !data.v_source || !data.rtpdepay || !data.demux || !data.video_queue||!data.video_parse || !data.v_decoder ||!data.video_convert|| !data.app_sink ||!data.app_queue|| !data.video_sink) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }
    
    /* Configure wavescope */
    //g_object_set (data.visual, "shader", 0, "style", 0, NULL);
    /* Set the URI to play */
    //g_object_set (data.av_source, "location", "/Users/Mudassar/Downloads/mux.ts", NULL);
    g_object_set (data.v_source, "uri" , "udp://localhost:5004", "caps", gst_caps_from_string("application/x-rtp") , NULL);
    
    
    
    
    
    /*Configure elements*/
    
    //g_object_set(data.video_sink, "location", "/Users/Mudassar/Downloads/demux.ts", NULL);
    
    
    
    
    
    /* Configure appsink */
    // g_object_set (data.app_sink, "emit-signals", TRUE, "caps", gst_caps_new_simple("meta/x-klv", "parsed", G_TYPE_BOOLEAN, TRUE, "sparse", G_TYPE_BOOLEAN, TRUE, NULL), NULL);
    
    // Try without "Sparse"
    g_object_set (data.app_sink, "emit-signals", TRUE, "caps", gst_caps_new_simple("meta/x-klv", "parsed", G_TYPE_BOOLEAN, TRUE, NULL), NULL);
    
    g_signal_connect (data.app_sink, "new-sample", G_CALLBACK (new_sample), &data);
    //gst_caps_unref (audio_caps);
    
    /* Link all elements that can be automatically linked because they have "Always" pads */
    gst_bin_add_many (GST_BIN (data.pipeline),  data.v_source, data.rtpdepay , data.demux, data.video_queue, data.video_parse, data.v_decoder, data.video_convert,   data.filter,  data.video_sink,data.app_queue, data.app_sink, NULL);
    
    
    
    GstCaps *depatCaps = gst_caps_from_string("video/mpegts");
    gboolean res = gst_element_link_filtered(data.rtpdepay, data.demux, depatCaps);
    gst_caps_unref(depatCaps);

   
 
    if (!gst_element_link_many (data.v_source, data.rtpdepay , NULL) || !gst_element_link_many(data.video_queue, data.video_parse, data.v_decoder, data.video_convert, data.filter, data.video_sink,  NULL) ||!gst_element_link (data.app_queue, data.app_sink) ) {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }
    
    
    
    filtercaps = gst_caps_new_simple ("video/x-raw",
                                      "format", G_TYPE_STRING, "NV12",
                                      "width", G_TYPE_INT, 854,
                                      "height", G_TYPE_INT, 480,
                                      //"framerate", GST_TYPE_FRACTION, 25, 1,
                                      NULL);
    g_object_set (G_OBJECT (data.filter), "caps", filtercaps, NULL);
    gst_caps_unref (filtercaps);
    
    
    /* filtercaps = gst_caps_new_simple ("video/x-raw",
     "width", G_TYPE_INT, 854,
     "height", G_TYPE_INT, 480,
     //"framerate", GST_TYPE_FRACTION, 25, 1,
     NULL);
     g_object_set (G_OBJECT (data.video_sink), "caps", filtercaps, NULL);
     gst_caps_unref (filtercaps);
     */
    
    
    /* Connect to the pad-added signal: Here we need to add this dignal to Demuxer. So we know when and what kind of pad is added  */
    g_signal_connect (data.demux, "pad-added", G_CALLBACK (pad_added_handler), &data);
    
    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    bus = gst_element_get_bus (data.pipeline);
    gst_bus_add_signal_watch (bus);
    g_signal_connect (G_OBJECT (bus), "message::error", (GCallback)error_cb, &data);
    
    
    
    
    gst_object_unref (bus);
    
    /* Start playing the pipeline */
    g_print ("  We are Starting the pipeline.\n");
    retr= gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
    if (retr == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }
    
    
    /* Listen to the bus */
    bus = gst_element_get_bus (data.pipeline);
    do {
        msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
                                          GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
        
        /* Parse message */
        if (msg != NULL) {
            GError *err;
            gchar *debug_info;
            
            switch (GST_MESSAGE_TYPE (msg)) {
                case GST_MESSAGE_ERROR:
                    gst_message_parse_error (msg, &err, &debug_info);
                    g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
                    g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
                    g_clear_error (&err);
                    g_free (debug_info);
                    terminate = TRUE;
                    break;
                case GST_MESSAGE_EOS:
                    g_print ("End-Of-Stream reached.\n");
                    terminate = TRUE;
                    break;
                case GST_MESSAGE_STATE_CHANGED:
                    /* We are only interested in state-changed messages from the pipeline */
                    if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data.pipeline)) {
                        GstState old_state, new_state, pending_state;
                        gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
                        g_print ("Pipeline state changed from %s to %s:\n",
                                 gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
                    }
                    break;
                default:
                    /* We should not reach here */
                    g_printerr ("Unexpected message received.\n");
                    break;
            }
            gst_message_unref (msg);
        }
    } while (!terminate);
    
    
    
    /* Create a GLib Main Loop and set it to run */
    data.main_loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (data.main_loop);
    
    /* Release the request pads from the Tee, and unref them */
    //gst_element_release_request_pad (data.tee, tee_audio_pad);
    //gst_element_release_request_pad (data.tee, tee_video_pad);
    //gst_element_release_request_pad (data.tee, tee_app_pad);
    // gst_object_unref (tee_audio_pad);
    //gst_object_unref (tee_video_pad);
    //gst_object_unref (tee_app_pad);
    
    /* Free resources */
    gst_element_set_state (data.pipeline, GST_STATE_NULL);
    gst_object_unref (data.pipeline);
    return 0;
}

static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data) {
    // GstPad *audio_sink_pad = gst_element_get_static_pad (data->audio_convert, "sink");
    
    GstPad *video_sink_pad = gst_element_get_static_pad (data->video_queue, "sink");
    GstPad *app_sink_pad = gst_element_get_static_pad (data->app_queue, "sink");
    
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;
    
    g_print ("\n Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));
    
    /* If our converter is already linked, we have nothing to do here */
    if (gst_pad_is_linked (app_sink_pad) && gst_pad_is_linked (video_sink_pad)) {
        g_print ("  We are already linked. Ignoring.\n");
        goto exit;
    }
    
    /* Check the new pad's type */
    new_pad_caps = gst_pad_query_caps (new_pad, NULL);
    new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
    new_pad_type = gst_structure_get_name (new_pad_struct);
    g_print ("  New pad type is:  '%s' ", new_pad_type);
    
    if (g_str_has_prefix (new_pad_type, "meta/x-klv") ) {
        /* Attempt the link */
        ret = gst_pad_link (new_pad, app_sink_pad);
        if (GST_PAD_LINK_FAILED (ret)) {
            g_print ("  Type is '%s' but link failed.\n", new_pad_type);
        } else {
            g_print ("  Link succeeded (type '%s').\n", new_pad_type);
        }
        
    } else if(g_str_has_prefix (new_pad_type, "video/x-h264"))
    {
        g_print ("  Attempting to link video pad\n");
        
        /* Attempt the  link */
        ret = gst_pad_link (new_pad, video_sink_pad);
        if (GST_PAD_LINK_FAILED (ret)) {
            g_print ("  Type is '%s' but link failed.\n", new_pad_type);
        } else {
            g_print ("  Link succeeded (type '%s').\n", new_pad_type);
        }
        
        
    }else goto exit;
    
    
    
exit:
    /* Unreference the new pad's caps, if we got them */
    if (new_pad_caps != NULL)
        gst_caps_unref (new_pad_caps);
    
    /* Unreference the sink pad */
    // gst_object_unref (audio_sink_pad);
    gst_object_unref (video_sink_pad);
    gst_object_unref(app_sink_pad);
}
