//
//  main.c
//  PushImage
//
//  Created by Ramin on 2017-08-24.
//  Copyright © 2017 Ramin. All rights reserved.
//

#include <stdio.h>
#include <gst/gst.h>
#include <string.h>
#include <stdlib.h>

static void
cb_need_data (GstElement *appsrc,
              guint       unused_size,
              gpointer    user_data);

unsigned char* buffer;
GMainLoop *loop;

int main(int argc, char * argv[]){
    GstElement *pipeline, *appsrc, *conv, *videosink;
    
    /* init GStreamer */
    gst_init (&argc, &argv);
    loop = g_main_loop_new (NULL, FALSE);
    
    /* setup pipeline */
    pipeline = gst_pipeline_new ("pipeline");
    appsrc = gst_element_factory_make ("appsrc", "source");
    conv = gst_element_factory_make ("videoconvert", "conv");
    videosink = gst_element_factory_make ("autovideosink", "videosink");
    
    /* setup */
    g_object_set (G_OBJECT (appsrc), "caps",
                  gst_caps_new_simple ("video/x-raw",
                                       "format", G_TYPE_STRING, "RGB",
                                       "width", G_TYPE_INT, 640,
                                       "height", G_TYPE_INT, 360,
                                       "framerate", GST_TYPE_FRACTION, 1, 1,
                                       NULL), NULL);
    gst_bin_add_many (GST_BIN (pipeline), appsrc, conv, videosink, NULL);
    gst_element_link_many (appsrc, conv, videosink, NULL);
    //g_object_set (videosink, "device", "/dev/video0", NULL);
    /* setup appsrc */
    g_object_set (G_OBJECT (appsrc),
                  "stream-type", 0,
                  "format", GST_FORMAT_TIME, NULL);
    g_signal_connect (appsrc, "need-data", G_CALLBACK (cb_need_data), NULL);
    
    /* play */
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    g_main_loop_run (loop);
    
    /* clean up */
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (GST_OBJECT (pipeline));
    g_main_loop_unref (loop);
    
    return 0;
}

void setBuffer(unsigned char* bytes){
    buffer = bytes;
}

char* readFile(char* fileName){
    char *fileData = NULL;
    FILE *f = fopen(fileName, "rb");
    if (f != NULL) {
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);  //same as rewind(f);
        
        fileData = malloc(fsize + 1);
        fread(fileData, fsize, 1, f);
        fclose(f);
        
        fileData[fsize] = 0;
    }
    return fileData;
}

static void
cb_need_data (GstElement *appsrc,
              guint       unused_size,
              gpointer    user_data)
{
    printf("cb_need_data\n");
    static gboolean white = FALSE;
    static GstClockTime timestamp = 0;
    GstBuffer *buffer;
    guint size,depth,height,width,step,channels;
    GstFlowReturn ret;
    //    IplImage* img;
    guchar *data1 = readFile("/Users/Ramin/Desktop/image.buf");
    GstMapInfo map;
    
    //    img=cvLoadImage("frame1.jpg",CV_LOAD_IMAGE_COLOR);
    height    = 10;
    width     = 10;
    //    step      = img->widthStep;
    channels  = 4;
    //    depth     = img->depth;
    //    data1      = (guchar *)img->imageData;
    size = height*width*channels;
    //
    buffer = gst_buffer_new_allocate (NULL, size, NULL);
    gst_buffer_map (buffer, &map, GST_MAP_WRITE);
    memcpy( (guchar *)map.data, data1,  gst_buffer_get_size( buffer ) );
    //    /* this makes the image black/white */
    gst_buffer_memset (buffer, 0, white ? 0xff : 0x0, size);
    
    white = !white;
    
    GST_BUFFER_PTS (buffer) = timestamp;
    GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 2);
    
    timestamp += GST_BUFFER_DURATION (buffer);
    
    g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);
    
    if (ret != GST_FLOW_OK) {
        /* something wrong, stop pushing */
        g_main_loop_quit (loop);
        printf("something went wrong!\n");
    }
    else
        printf("pushed somehow successfully!\n");
}
