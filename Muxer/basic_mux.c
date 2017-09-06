//
//  basic-mux.c
//  GStreamer Tutorials
//
//  Created by Mudassar on 25/10/16.
//
//


#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <string.h>
#include <gst/app/gstappsrc.h>
#include <zmq.h>
#include <msgpack.h>

#define CHUNK_SIZE 151  /* Amount of bytes we are sending in each buffer */
#define SAMPLE_RATE 25 /* Samples per second we are sending */
gchar metabuff[150]= "METADATA: ";
int frame_counter = 0;
int video_width = 960;
int video_height = 540;


/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
    GstElement *pipeline, *app_source, *v_source, *av_source, *filter, *filter2, *audio_convert,*video_convert2 , *audio_resample, *audio_queue; //*tee, *audio_sink, *audio_queue;
    GstElement  *video_convert, *video_queue; //, *audio_convert,,*video_queue,*video_sink;
    GstElement *muxer, *buffer_queue, *tsparser, *v_encoder,*a_encoder, *udpPayloader,*payloader_queue, *udp_sink,  *file_sink,  *app_queue, *rtpjitter;//,*app_sink;
    
    
    
    guint64 num_samples;   /* Number of samples generated so far (for timestamp generation) */
    gfloat a, b, c, d;     /* For waveform generation */
    
    guint sourceid;        /* To control the GSource */
    
    GMainLoop *main_loop;  /* GLib's Main Loop */
    
    
    
} CustomData;


void getmeta (guint8 *, int, int );


void getmeta(guint8 * ptr, int width, int height){
    
    
    
    // printf ("Connecting to hello world server…\n");
    void *context = zmq_ctx_new ();
    void *requester = zmq_socket (context, ZMQ_REQ);
    zmq_connect (requester, "tcp://localhost:5555");
    
    
    /* Massage pack code*/
    
    /* creates buffer and serializer instance. */
    msgpack_sbuffer* mp_buffer = msgpack_sbuffer_new();
    msgpack_packer* pk = msgpack_packer_new(mp_buffer, msgpack_sbuffer_write);
    
    /* serializes ["Hello", "MessagePack"]. */
    
    
    msgpack_pack_array(pk, width*height*3/2);
    
    //msgpack_pack_int16(pk, 33);
    //msgpack_pack_int16(pk, 44);
    int y, x, z;
    //for (z=0; z<3; z++)
    //      for (y = 0; y < height; y++) {
    for (x = 0; x < width*height*3/2; x++)
    {
        
        msgpack_pack_uint8(pk, ptr[x]);
    }
    
    //        ptr += height;
    //  }
    
    
    
    // msgpack_pack_int16(pk, arr[1]);
    
    
    
    //msgpack_pack_bin_body(pk, arr, 32);
    //    msgpack_pack_bin(pk, 5);
    //    msgpack_pack_bin_body(pk, "Hellpow", 5);
    //    msgpack_pack_bin(pk, 11);
    //    msgpack_pack_bin_body(pk, "MessagePack", 11);
    
    
    
    //    zmq_msg_t msg;
    //    //zmq_msg_init_data(&msg, mp_buffer->data, mp_buffer->size, free_msgpack_msg,
    //     //                 mp_buffer);
    //    zmq_msg_init_size(&msg, sizeof(mp_buffer));
    //    memcpy(&msg, mp_buffer->data, mp_buffer->size);
    //    zmq_msg_send(&msg, requester, ZMQ_DONTWAIT);
    
    zmq_send(requester, mp_buffer->data, mp_buffer->size, 0);
    zmq_recv (requester, metabuff, 150, 0);
    printf ("Received:...\n");
    
    
    
    
    zmq_close (requester);
    zmq_ctx_destroy (context);
    
    /* cleaning msgpack */
    msgpack_sbuffer_free(mp_buffer);
    msgpack_packer_free(pk);
    
    
    
    
}





static GstPadProbeReturn
cb_have_data (GstPad          *pad,
              GstPadProbeInfo *info,
              gpointer         user_data)
{
    gint x, y;
    GstMapInfo map;
    guint8 *ptr, t;
    GstBuffer *buffer;
    
    
    buffer = GST_PAD_PROBE_INFO_BUFFER (info);
    
    buffer = gst_buffer_make_writable (buffer);
    // g_print("probed buffer PTS: %lu \n" , buffer->pts/1000000000 );
    
    /* Making a buffer writable can fail (for example if it
     * cannot be copied and is used more than once)
     */
    if (buffer == NULL)
        return GST_PAD_PROBE_OK;
    
    /* Mapping a buffer can fail (non-writable) */
    if (gst_buffer_map (buffer, &map, GST_MAP_WRITE)) {
        ptr = (guint8 *) map.data;
        
        /* invert data */
        
        //g_usleep(20000);
        
        if(frame_counter >= 5)
        {
            //getmeta (ptr, video_width, video_height);
            frame_counter=0;
            
            
        }
        frame_counter ++;
        
        
        //g_print("maped biffer size: %d\n" , (int)map.size);
        
        
        
        gst_buffer_unmap (buffer, &map);
    }
    
    
    GST_PAD_PROBE_INFO_DATA (info) = buffer;
    
    return GST_PAD_PROBE_OK;
    
}





/* This method is called by the idle GSource in the mainloop, to feed CHUNK_SIZE bytes into appsrc.
 * The idle handler is added to the mainloop when appsrc requests us to start sending data (need-data signal)
 * and is removed when appsrc has enough data (enough-data signal).
 */
static gboolean push_data (CustomData *data) {
    GstBuffer *buffer;
    GstFlowReturn ret;
    int i;
    GstMapInfo map;
    //gint16 *raw;
    gchar *raw;
    gint num_samples = CHUNK_SIZE ; /* Because each sample is 16 bits */
    static GstClockTime timestamp = 0;
    
    //    gchar x[] = "{\"rects\": [[840, 208, 940, 486], [0, 60, 72, 219], [45, 110, 165, 305], [182, 231, 302, 435], [691, 172, 815, 374]], \"index\": 5000}";
    //
    //g_print("push-data\n");
    
    /* Create a new empty buffer */
    buffer = gst_buffer_new_and_alloc (CHUNK_SIZE);
    
    /* Set its timestamp and duration */
    //GST_BUFFER_TIMESTAMP (buffer) = gst_util_uint64_scale (data->num_samples, GST_SECOND, SAMPLE_RATE);
    //g_print ("GST_BUFFER_Timestamp: %d",GST_BUFFER_TIMESTAMP(buffer) );
    //GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale (CHUNK_SIZE, GST_SECOND, SAMPLE_RATE);
    
    GST_BUFFER_TIMESTAMP (buffer) = timestamp;
    GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 1);
    
    timestamp += GST_BUFFER_DURATION (buffer);
    
    /* Generate some psychodelic waveforms */
    gst_buffer_map (buffer, &map, GST_MAP_WRITE);
    //raw = (gint16 *)map.data;
    raw = (gchar *)map.data;
    //data->c += data->d;
    //data->d -= data->c / 1000;
    //freq = 1100 + 1000 * data->d;
    //    for (i = 0; i < num_samples; i++) {
    //        //data->a += data->b;
    //        //data->b -= data->a / freq;
    //
    //        //raw[i] = (gint16)(500 * data->a);
    //        raw[i] = 'b';//(gint16)gst_util_get_timestamp();
    //        //g_print("* \n");
    //    }
    strcpy(raw, metabuff);
    //g_print("metabuff: %s \n", metabuff);
    //for (i = 0; i < num_samples; i++) {
    
    //raw[i] = (gint16)x[i];
    
    //}
    
    
    gst_buffer_unmap (buffer, &map);
    data->num_samples += num_samples;
    
    
    /* Push the buffer into the appsrc */
    g_signal_emit_by_name (data->app_source, "push-buffer", buffer, &ret);
    
    /* Free the buffer now that we are done with it */
    gst_buffer_unref (buffer);
    
    if (ret != GST_FLOW_OK) {
        /* We got some error, stop sending data */
        return FALSE;
    }
    
    return TRUE;
}

/* This signal callback triggers when appsrc needs data. Here, we add an idle handler
 * to the mainloop to start pushing data into the appsrc */
static void start_feed (GstElement *source, guint size, CustomData *data) {
    // only push buffer when we have something in metabuff
    
    if ( data->sourceid == 0) {
        g_print ("Start feeding\n");
        data->sourceid = g_idle_add ((GSourceFunc) push_data, data);
    }
}

/* This callback triggers when appsrc has enough data and we can stop sending.
 * We remove the idle handler from the mainloop */
static void stop_feed (GstElement *source, CustomData *data) {
    if (data->sourceid != 0) {
        g_print ("Stop feeding\n");
        g_source_remove (data->sourceid);
        data->sourceid = 0;
    }
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
    GstBus *bus;
    GstCaps *filtercaps;
    
    /* Initialize cumstom data structure */
    memset (&data, 0, sizeof (data));
    data.b = 1; /* For waveform generation */
    data.d = 1;
    
    
    GstPad *pad;
    
    /* Initialize GStreamer */
    gst_init (&argc, &argv);
    
    /* Create the elements */
    data.app_source = gst_element_factory_make ("appsrc", "app_source");
    //data.audio_queue = gst_element_factory_make ("queue", "audio_queue");
    data.audio_convert = gst_element_factory_make ("audioconvert", "audio_convert");
    data.audio_resample = gst_element_factory_make ("audioresample", "audio_resample");
    //data.audio_sink = gst_element_factory_make ("autoaudiosink", "audio_sink");
    data.video_queue = gst_element_factory_make ("queue", "video_queue");
    // data.audio_convert2 = gst_element_factory_make ("audioconvert", "audio_convert2");
    
    data.video_convert = gst_element_factory_make ("videoconvert", "video_convert");
    
    data.video_convert2 = gst_element_factory_make ("videoconvert", "video_convert2");
    // data.video_sink = gst_element_factory_make ("autovideosink", "video_sink");
    data.app_queue = gst_element_factory_make ("queue", "app_queue");
    
    data.muxer = gst_element_factory_make("mpegtsmux", "tsmux");
    data.tsparser = gst_element_factory_make("tsparse", "tsparser" );
    
    
    
    data.v_source = gst_element_factory_make ("avfvideosrc", "v_source");
    data.av_source = gst_element_factory_make ("uridecodebin", "av_source");
    
    
    data.filter = gst_element_factory_make ("capsfilter", "filter");
    data.filter2 = gst_element_factory_make ("capsfilter", "filter2");
    
    data.v_encoder = gst_element_factory_make("x264enc", "v_encoder");
    data.a_encoder = gst_element_factory_make("voaacenc", "a_encoder");
    data.payloader_queue =gst_element_factory_make ("queue", "payloader_queue");
    data.buffer_queue =gst_element_factory_make ("queue", "buffer_queue");
    data.udpPayloader =gst_element_factory_make("rtpmp2tpay", "udpPayloader");
    data.rtpjitter =gst_element_factory_make ("rtpjitterbuffer", "rtpjitter");
    data.udp_sink =gst_element_factory_make("udpsink", "udp_sink");
    data.file_sink =gst_element_factory_make("filesink", "file_sink");
    
    
    
    /* Create the empty pipeline */
    data.pipeline = gst_pipeline_new ("test-pipeline");
    
    if (!data.pipeline || !data.app_source   || !data.audio_convert ||
        !data.video_convert || !data.muxer ||! data.buffer_queue || !data.tsparser || !data.av_source || ! data.filter ||  !data.file_sink || !data.a_encoder || !data.v_encoder || !data.audio_resample || ! data.video_convert2 || !data.filter2|| ! data.udpPayloader || ! data.payloader_queue  || ! data.udp_sink) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }
    
    /* Configure wavescope */
    //g_object_set (data.visual, "shader", 0, "style", 0, NULL);
    /* Set the URI to play  "file:///Users/Mudassar/Downloads/trial.webm or rtsp://vision-mixer.sics.se:1935/live/livelink2*/
    g_object_set (G_OBJECT(data.av_source), "uri", "file:///Users/Ramin/Downloads/whyred_0_540p.mov", NULL);
    
    /*Configure elements*/
    
    g_object_set(data.file_sink, "location", "/Users/Mudassar/Downloads/mux.ts", NULL);
    
    //g_object_set(data.av_source,  "use-buffering", TRUE, "download" , TRUE,"buffer-duration" , (guint64)30000000000, NULL);
    
    
    //GstCaps *gstAVFCaps = gst_caps_from_string("video/x-raw-yuv, framerate=25/1, do-timestamp=true");
    //gst_element_link_filtered(data.av_source, data.video_convert, gstAVFCaps);
    //gst_caps_unref(gstAVFCaps);
    
    //g_object_set(G_OBJECT(data.v_encoder),"bitrate", 1500, "tune", 0x00000004, "byte-stream", TRUE, "key-int-max", 25,   NULL);
    g_object_set(G_OBJECT(data.v_encoder),"bitrate", 2000, "byte-stream", TRUE, "key-int-max", 25, "threads", 4,   NULL);
    g_object_set(G_OBJECT(data.muxer),"m2ts-mode", FALSE, "pat-interval", 3000, "pmt-interval", 3000,   NULL);
    g_object_set(G_OBJECT(data.tsparser),"set-timestamps", TRUE, NULL);
    g_object_set(G_OBJECT(data.udp_sink),"port", 5004, "host", "localhost","sync", TRUE,   NULL);
    
    // g_object_set(G_OBJECT(data.rtpjitter),"latency", (guint)3000,   NULL);
    // g_object_set(G_OBJECT(data.video_queue),"max-size-time", (guint64)10000000000, "min-threshold-time",(guint64)5000000000 ,   NULL);
    // g_object_set(G_OBJECT(data.buffer_queue),"max_size_buffers", 0, "max_size_bytes", 0, "max-size-time", (guint64)30000000000,     NULL);
    g_object_set(G_OBJECT(data.udp_sink),"ts-offset", 120000000,   NULL);
    
    // g_object_set(G_OBJECT(data.video_queue),"leaky", 1,    NULL);
    
    
    
    
    /* Configure appsrc */
    //gst_audio_info_set_format (&info, GST_AUDIO_FORMAT_S16, SAMPLE_RATE, 1, NULL);
    gst_app_src_set_caps (GST_APP_SRC (data.app_source), gst_caps_new_simple("meta/x-klv", "parsed", G_TYPE_BOOLEAN, TRUE, "sparse", G_TYPE_BOOLEAN, TRUE, NULL));
    g_object_set(data.app_source, "format", GST_FORMAT_TIME, "is-live", TRUE,"max-bytes",(guint64)300, "do-timestamp", FALSE, NULL);
    //audio_caps = gst_audio_info_to_caps (&info);
    //g_object_set (data.app_source, "caps", audio_caps, "format", GST_FORMAT_TIME, NULL);
    g_signal_connect (data.app_source, "need-data", G_CALLBACK (start_feed), &data);
    g_signal_connect (data.app_source, "enough-data", G_CALLBACK (stop_feed), &data);
    
    
    
    /* Configure appsink */
    // g_object_set (data.app_sink, "emit-signals", TRUE, "caps", audio_caps, NULL);
    // g_signal_connect (data.app_sink, "new-sample", G_CALLBACK (new_sample), &data);
    // gst_caps_unref (audio_caps);
    
    /* Link all elements that can be automatically linked because they have "Always" pads */
    gst_bin_add_many (GST_BIN (data.pipeline),  data.app_source, data.app_queue,  data.av_source, data.video_queue, data.filter, data.video_convert, data.video_convert2, data.filter2, data.v_encoder, data.muxer,data.buffer_queue, data.tsparser, data.payloader_queue, data.udpPayloader,data.rtpjitter, data.udp_sink, NULL);
    
    
    ////
    //    gst_bin_add_many (GST_BIN (data.pipeline),  data.app_source, data.app_queue,  data.av_source, data.video_queue, data.filter, data.video_convert, data.video_convert2, data.filter2, data.v_encoder, data.tsmux,data.buffer_queue, data.file_sink, NULL);
    
    
    
    if (!gst_element_link_many ( data.video_convert,data.filter, data.video_queue, data.v_encoder, data.muxer, NULL) || !gst_element_link_many(data.app_source, data.app_queue, data.muxer, NULL)|| !gst_element_link_many(data.muxer, data.tsparser, data.payloader_queue,  data.udpPayloader, data.buffer_queue, data.udp_sink, NULL)  ) {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }
    
    
    //        if (!gst_element_link_many ( data.video_convert,data.filter, data.video_queue,data.video_convert2, data.filter2, data.v_encoder, data.tsmux, NULL) || !gst_element_link_many(data.app_source, data.app_queue, data.tsmux, NULL)|| !gst_element_link_many(data.tsmux,data.file_sink ,NULL)  ) {
    //            g_printerr ("Elements could not be linked.\n");
    //            gst_object_unref (data.pipeline);
    //            return -1;
    //        }
    //
    
    filtercaps = gst_caps_new_simple ("video/x-raw",
                                      "format", G_TYPE_STRING, "I420",
                                      "width", G_TYPE_INT, video_width,
                                      "height", G_TYPE_INT, video_height,
                                      //"framerate", GST_TYPE_FRACTION, 10, 1.0, //this line causes "0 as denominator" error
                                      NULL);
    g_object_set (G_OBJECT (data.av_source), "caps", filtercaps, NULL);
    gst_caps_unref (filtercaps);
    
    //    filtercaps = gst_caps_new_simple ("video/x-raw",
    //                                      "format", G_TYPE_STRING, "NV12",
    //                                       "width", G_TYPE_INT, video_width,
    //                                       "height", G_TYPE_INT, video_height,
    //                                      // "framerate", GST_TYPE_FRACTION, 25, 1,
    //                                      NULL);
    //    g_object_set (G_OBJECT (data.filter2), "caps", filtercaps, NULL);
    //    gst_caps_unref (filtercaps);
    //
    
    
    pad = gst_element_get_static_pad (data.filter  , "src");
    
    if (pad == NULL)
        g_print("no pad found /n");
    
    gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BUFFER,
                       (GstPadProbeCallback) cb_have_data, NULL, NULL);
    
    
    gst_object_unref (pad);
    
    /* Connect to the pad-added signal */
    g_signal_connect (data.av_source, "pad-added", G_CALLBACK (pad_added_handler), &data);
    
    
    /* Manually link the Tee, which has "Request" pads */
    /*
     tee_src_pad_template = gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (data.tee), "src_%u");
     tee_audio_pad = gst_element_request_pad (data.tee, tee_src_pad_template, NULL, NULL);
     g_print ("Obtained request pad %s for audio branch.\n", gst_pad_get_name (tee_audio_pad));
     queue_audio_pad = gst_element_get_static_pad (data.audio_queue, "sink");
     tee_video_pad = gst_element_request_pad (data.tee, tee_src_pad_template, NULL, NULL);
     g_print ("Obtained request pad %s for video branch.\n", gst_pad_get_name (tee_video_pad));
     queue_video_pad = gst_element_get_static_pad (data.video_queue, "sink");
     tee_app_pad = gst_element_request_pad (data.tee, tee_src_pad_template, NULL, NULL);
     g_print ("Obtained request pad %s for app branch.\n", gst_pad_get_name (tee_app_pad));
     queue_app_pad = gst_element_get_static_pad (data.app_queue, "sink");
     if (gst_pad_link (tee_audio_pad, queue_audio_pad) != GST_PAD_LINK_OK ||
     gst_pad_link (tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK ||
     gst_pad_link (tee_app_pad, queue_app_pad) != GST_PAD_LINK_OK) {
     g_printerr ("Tee could not be linked\n");
     gst_object_unref (data.pipeline);
     return -1;
     }
     gst_object_unref (queue_audio_pad);
     gst_object_unref (queue_video_pad);
     gst_object_unref (queue_app_pad);
     */
    /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
    bus = gst_element_get_bus (data.pipeline);
    gst_bus_add_signal_watch (bus);
    g_signal_connect (G_OBJECT (bus), "message::error", (GCallback)error_cb, &data);
    gst_object_unref (bus);
    
    /* Start playing the pipeline */
    gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
    
    /* Create a GLib Main Loop and set it to run */
    data.main_loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (data.main_loop);
    
    
    
    /* Free resources */
    gst_element_set_state (data.pipeline, GST_STATE_NULL);
    gst_object_unref (data.pipeline);
    return 0;
}

static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data) {
    //GstPad *audio_sink_pad = gst_element_get_static_pad (data->audio_convert, "sink");
    GstPad *video_sink_pad = gst_element_get_static_pad (data->video_convert, "sink");
    
    GstPadLinkReturn ret;
    GstCaps *new_pad_caps = NULL;
    GstStructure *new_pad_struct = NULL;
    const gchar *new_pad_type = NULL;
    
    g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));
    
    /* If our converter is already linked, we have nothing to do here */
    if ( gst_pad_is_linked (video_sink_pad)) {
        g_print ("  We are already linked. Ignoring.\n");
        goto exit;
    }
    
    /* Check the new pad's type */
    new_pad_caps = gst_pad_query_caps (new_pad, NULL);
    new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
    new_pad_type = gst_structure_get_name (new_pad_struct);
    if (g_str_has_prefix (new_pad_type, "video/x-raw") ) {
        /* Attempt the link */
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
}
