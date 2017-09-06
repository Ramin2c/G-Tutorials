//
//  jpegmux.c
//  GStreamer Tutorials
//
//  Created by Mudassar on 2017-05-30.
//
//




#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <string.h>
#include <gst/app/gstappsrc.h>


#define CHUNK_SIZE 151  /* Amount of bytes we are sending in each buffer */
#define SAMPLE_RATE 25 /* Samples per second we are sending */
gchar metabuff[150]= "1 \n 00:00:30,150 --> 00:00:45,109  \n Never drink liquid nitrogen";


int frame_counter = 0;
int video_width = 1280;//854;
int video_height = 720;//480;


/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
    GstElement *pipeline,  *v_source, *av_source, *filter, *filter2, *video_convert2 ; //*tee, *audio_sink, *audio_queue;
    GstElement  *video_convert, *video_queue, *tcpsinkclient, *multipartmuxer; //, *audio_convert,,*video_queue,*video_sink;
    GstElement *muxer, *taginjector , *buffer_queue,  *v_encoder,  *udp_sink,  *file_sink,  *app_queue;
    
    
    
    
    guint sourceid;        /* To control the GSource */
    
    GMainLoop *main_loop;  /* GLib's Main Loop */
    
    
    
} CustomData;

//
// void getmeta (guint8 *, int, int );
//
//
// void getmeta(guint8 * ptr, int width, int height){
//
//
//
//     // printf ("Connecting to hello world server…\n");
//     void *context = zmq_ctx_new ();
//     void *requester = zmq_socket (context, ZMQ_REQ);
//     zmq_connect (requester, "tcp://localhost:5555");
//
//
//     /* Massage pack code*/
//
//     /* creates buffer and serializer instance. */
//     msgpack_sbuffer* mp_buffer = msgpack_sbuffer_new();
//     msgpack_packer* pk = msgpack_packer_new(mp_buffer, msgpack_sbuffer_write);
//
//     /* serializes ["Hello", "MessagePack"]. */
//
//
//     msgpack_pack_array(pk, width*height*3/2);
//
//     //msgpack_pack_int16(pk, 33);
//     //msgpack_pack_int16(pk, 44);
//     int y, x, z;
//     //for (z=0; z<3; z++)
//     //      for (y = 0; y < height; y++) {
//     for (x = 0; x < width*height*3/2; x++)
//     {
//
//         msgpack_pack_uint8(pk, ptr[x]);
//     }
//
//     //        ptr += height;
//     //  }
//
//
//
//     // msgpack_pack_int16(pk, arr[1]);
//
//
//
//     //msgpack_pack_bin_body(pk, arr, 32);
//     //    msgpack_pack_bin(pk, 5);
//     //    msgpack_pack_bin_body(pk, "Hellpow", 5);
//     //    msgpack_pack_bin(pk, 11);
//     //    msgpack_pack_bin_body(pk, "MessagePack", 11);
//
//
//
//     //    zmq_msg_t msg;
//     //    //zmq_msg_init_data(&msg, mp_buffer->data, mp_buffer->size, free_msgpack_msg,
//     //     //                 mp_buffer);
//     //    zmq_msg_init_size(&msg, sizeof(mp_buffer));
//     //    memcpy(&msg, mp_buffer->data, mp_buffer->size);
//     //    zmq_msg_send(&msg, requester, ZMQ_DONTWAIT);
//
//     zmq_send(requester, mp_buffer->data, mp_buffer->size, 0);
//     zmq_recv (requester, metabuff, 150, 0);
//     printf ("Received:...\n");
//
//
//
//
//     zmq_close (requester);
//     zmq_ctx_destroy (context);
//
//     /* cleaning msgpack */
//     msgpack_sbuffer_free(mp_buffer);
//     msgpack_packer_free(pk);
//
//
//
//
// }
//



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
    
    //g_print("probed buffer PTS: %lu\n" , buffer->pts/1000);
    
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
        
        if(frame_counter >= 3)
        {
            
            // getmeta (ptr, video_width, video_height);
            frame_counter=0;
            
            // this is only a trial code to see how srt formatted subtitle work with this pipeplien
            g_usleep(10000); // microseconds
            
        }
        frame_counter ++;
        
        
        //g_print("maped biffer size: %d\n" , (int)map.size);
        
        GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);
        //g_print ("Got event: %s\n", GST_EVENT_TYPE_NAME (event));
        gboolean ret;
        
        GstTagList *taglist ;
        //gchar *tags =GST_TAG_TITLE "title,this is you";
        //taglist= gst_tag_list_new_from_string(tags);
        taglist=gst_tag_list_new("artist","a quick brown fox jumps over the lazy doga quick brown fox jumps over the lazy doga quick brown fox jumps over the lazy dog", NULL);
        
        // taglist= GST_TAG_LIST (gst_structure_from_string ((GST_TAG_COMMENT)"title,test_title", NULL));
        //g_print("******\n");
        //g_print("%s", gst_tag_list_to_string(taglist));
        // event= gst_event_new_tag (taglist);
        gst_pad_push_event (pad, gst_event_new_tag (GST_TAG_LIST(taglist)));
        gst_tag_list_ref(taglist);
        //ret = gst_pad_push_event (pad, event);
        
        
        
        
        gst_buffer_unmap (buffer, &map);
    }
    
    
    GST_PAD_PROBE_INFO_DATA (info) = buffer;
    
    return GST_PAD_PROBE_OK;
    
}



static GstPadProbeReturn
cb_event(GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
    GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);
    g_print ("Got event: %s\n", GST_EVENT_TYPE_NAME (event));
    gboolean ret;
    
    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) {
        if (GST_EVENT_TYPE (event) == GST_EVENT_TAG) {
            
            GstTagList *taglist ;
            taglist= gst_tag_list_new_from_string("title=abcdefghijklmn");
            
            //taglist = gst_tag_list_new (GST_TAG_COMMENT, "test-comment", NULL);
            
            event= gst_event_new_tag (taglist);
            //gst_tag_list_insert (taglist, gst_event_tag_get_list (event),
            //   GST_TAG_MERGE_PREPEND);
            
            ret = gst_pad_push_event (pad, event);
            
        }
    }
    
    
    //GstTagList *taglist = gst_tag_list_new_from_string("title=abcdefghijklmn");
    //gst_event_new_tag (taglist);
    //gst_tag_list_insert (taglist, gst_event_tag_get_list (event),
    //   GST_TAG_MERGE_PREPEND);
    
    ret = gst_pad_push_event (pad, event);
    
    
    
    
    
    return GST_PAD_PROBE_OK;
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
    
    
    GstPad *pad;
    
    
    /* Initialize GStreamer */
    gst_init (&argc, &argv);
    
    /* Create the elements */
    
    data.video_queue = gst_element_factory_make ("queue", "video_queue");
    
    data.video_queue = gst_element_factory_make ("taginject", "taginjector");
    
    
    
    data.video_convert = gst_element_factory_make ("videoconvert", "video_convert");
    
    data.video_convert2 = gst_element_factory_make ("videoconvert", "video_convert2");
    data.app_queue = gst_element_factory_make ("queue", "app_queue");
    
    data.muxer = gst_element_factory_make("jifmux", "muxer");
    
    
    
    data.v_source = gst_element_factory_make ("avfvideosrc", "v_source");
    
    data.filter = gst_element_factory_make ("capsfilter", "filter");
    data.filter2 = gst_element_factory_make ("capsfilter", "filter2");
    
    data.v_encoder = gst_element_factory_make("jpegenc", "v_encoder");
    data.buffer_queue =gst_element_factory_make ("queue", "buffer_queue");
    data.file_sink =gst_element_factory_make("multifilesink", "file_sink");
    data.tcpsinkclient=gst_element_factory_make("tcpclientsink", "tcpsinkclient");
    
    data.multipartmuxer=gst_element_factory_make("multipartmux", "multipartmuxer");
    
    
    
    /* Create the empty pipeline */
    data.pipeline = gst_pipeline_new ("test-pipeline");
    
    if (!data.pipeline    || !data.video_convert || !data.muxer ||! data.buffer_queue || !data.v_source || ! data.filter ||  !data.file_sink ||  !data.v_encoder || ! data.video_convert2 || !data.filter2 || ! data.tcpsinkclient || ! data.multipartmuxer ) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }
    
    /* Configure wavescope */
    /* Set the URI to play  "file:///Users/Mudassar/Downloads/trial.webm or rtsp://vision-mixer.sics.se:1935/live/livelink2*/
    // g_object_set (G_OBJECT(data.av_source), "uri", "file:///Users/Mudassar/Downloads/trial.webm", NULL);
    
    /*Configure elements*/
    
    g_object_set(data.file_sink, "location", "/Users/Mudassar/Downloads/test.jpeg", "sync", TRUE, NULL);
    g_object_set(G_OBJECT(data.taginjector), "tags",  "title=somethingsomething", NULL);
    
    g_object_set(G_OBJECT(data.tcpsinkclient), "host",  "127.0.0.1", "port", 9999,  NULL);
    g_object_set(G_OBJECT(data.multipartmuxer), "boundary",  "spionisto", NULL);
    
    
    
    
    
    //g_object_set(data.av_source,  "use-buffering", TRUE, "download" , TRUE,"buffer-duration" , (guint64)30000000000, NULL);
    
    
    //GstCaps *gstAVFCaps = gst_caps_from_string("video/x-raw-yuv, framerate=25/1, do-timestamp=true");
    //gst_element_link_filtered(data.av_source, data.video_convert, gstAVFCaps);
    //gst_caps_unref(gstAVFCaps);
    
    
    
    g_object_set(G_OBJECT(data.udp_sink),"port", 5004, "host", "localhost","sync", TRUE,   NULL);
    
    // g_object_set(G_OBJECT(data.video_queue),"max-size-time", (guint64)10000000000, "min-threshold-time",(guint64)5000000000 ,   NULL);
    // g_object_set(G_OBJECT(data.buffer_queue),"max_size_buffers", 0, "max_size_bytes", 0, "max-size-time", (guint64)30000000000,     NULL);
    g_object_set(G_OBJECT(data.udp_sink),"ts-offset", 120000000,   NULL);
    
    
    
    
    /* Configure appsrc */
    
    
    
    gst_bin_add_many (GST_BIN (data.pipeline), data.v_source, data.app_queue, data.video_queue, data.filter, data.video_convert, data.video_convert2, data.filter2, data.v_encoder, data.muxer,data.buffer_queue, data.multipartmuxer, data.tcpsinkclient, NULL);
    
    
    
    if (!gst_element_link_many ( data.v_source,  data.filter , data.video_convert2 , data.v_encoder,  data.buffer_queue, data.muxer , data.multipartmuxer, data.tcpsinkclient, NULL)  ) {
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (data.pipeline);
        return -1;
    }
    
    
    
    
    
    filtercaps = gst_caps_new_simple ("video/x-raw",
                                      "format", G_TYPE_STRING, "NV12",
                                      "width", G_TYPE_INT, 1280,
                                      "height", G_TYPE_INT, 720,
                                      "framerate", GST_TYPE_FRACTION, 25, 1,
                                      NULL);
    g_object_set (G_OBJECT (data.filter), "caps", filtercaps, NULL);
    gst_caps_unref (filtercaps);
    
    filtercaps = gst_caps_new_simple ("image/jpeg",
                                      //"format", G_TYPE_STRING, "NV12",
                                      "width", G_TYPE_INT, 640,
                                      "height", G_TYPE_INT, 480,
                                      //"framerate", GST_TYPE_FRACTION, 25, 1,
                                      NULL);
    g_object_set (G_OBJECT (data.filter2), "caps", filtercaps, NULL);
    gst_caps_unref (filtercaps);
    
    
    
    pad = gst_element_get_static_pad (data.filter  , "src");
    
    if (pad == NULL)
        g_print("no pad found /n");
    
    gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BUFFER,
                       (GstPadProbeCallback) cb_have_data, NULL, NULL);
    
    
    
    gst_object_unref (pad);
    
    
    /* Connect to the pad-added signal */
    // g_signal_connect (data.av_source, "pad-added", G_CALLBACK (pad_added_handler), &data);
    
    
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
