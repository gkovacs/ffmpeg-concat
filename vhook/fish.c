/*
 * Fish Detector Hook
 * Copyright (c) 2002 Philip Gladstone
 *
 * This file implements a fish detector. It is used to see when a 
 * goldfish passes in front of the camera. It does this by counting
 * the number of input pixels that fall within a particular HSV
 * range.
 *
 * It takes a multitude of arguments:
 *
 * -h <num>-<num>    the range of H values that are fish
 * -s <num>-<num>    the range of S values that are fish
 * -v <num>-<num>    the range of V values that are fish
 * -z                zap all non-fish values to black
 * -l <num>          limit the number of saved files to <num>
 * -i <num>          only check frames every <num> seconds
 * -t <num>          the threshold for the amount of fish pixels (range 0-1)
 * -d                turn debugging on
 * -D <directory>    where to put the fish images
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <stdio.h>
#include <dirent.h>

#include "framehook.h"
#include "dsputil.h"

#define SCALE_BITS 10
 
#define C_Y  (76309 >> (16 - SCALE_BITS))
#define C_RV (117504 >> (16 - SCALE_BITS))
#define C_BU (138453 >> (16 - SCALE_BITS))
#define C_GU (13954 >> (16 - SCALE_BITS))
#define C_GV (34903 >> (16 - SCALE_BITS))                                       

  
typedef struct {
    int h;  /* 0 .. 360 */
    int s;  /* 0 .. 255 */
    int v;  /* 0 .. 255 */
} HSV;
              
typedef struct {
    int zapping;
    int threshold;
    HSV dark, bright;
    char *dir;
    int file_limit;
    int debug;
    int min_interval;
    INT64 next_pts;
    int inset;
    int min_width;
} ContextInfo;

static void dorange(const char *s, int *first, int *second, int maxval)
{
    sscanf(s, "%d-%d", first, second);
    if (*first > maxval)
        *first = maxval;
    if (*second > maxval)
        *second = maxval;
}


int Configure(void **ctxp, int argc, char *argv[])
{
    ContextInfo *ci;
    int c;

    *ctxp = av_mallocz(sizeof(ContextInfo));
    ci = (ContextInfo *) *ctxp;

    optind = 0;

    ci->dir = "/tmp";
    ci->threshold = 1000;
    ci->file_limit = 100;
    ci->min_interval = 1000000;
    ci->inset = 10;     /* Percent */

    while ((c = getopt(argc, argv, "w:i:dh:s:v:zl:t:D:")) > 0) {
        switch (c) {
            case 'h':
                dorange(optarg, &ci->dark.h, &ci->bright.h, 360);
                break;
            case 's':
                dorange(optarg, &ci->dark.s, &ci->bright.s, 255);
                break;
            case 'v':
                dorange(optarg, &ci->dark.v, &ci->bright.v, 255);
                break;
            case 'z':
                ci->zapping = 1;
                break;
            case 'l':
                ci->file_limit = atoi(optarg);
                break;
            case 'i':
                ci->min_interval = 1000000 * atof(optarg);
                break;
            case 't':
                ci->threshold = atof(optarg) * 1000;
                break;
            case 'w':
                ci->min_width = atoi(optarg);
                break;
            case 'd':
                ci->debug++;
                break;
            case 'D':
                ci->dir = strdup(optarg);
                break;
            default:
                fprintf(stderr, "Unrecognized argument '%s'\n", argv[optind]);
                return -1;
        }
    }

    fprintf(stderr, "Fish detector configured:\n");
    fprintf(stderr, "    HSV range: %d,%d,%d - %d,%d,%d\n",
                        ci->dark.h,
                        ci->dark.s,
                        ci->dark.v,
                        ci->bright.h,
                        ci->bright.s,
                        ci->bright.v);

    return 0;
}

static void get_hsv(HSV *hsv, int r, int g, int b)
{
    int i, v, x, f;
         
    x = (r < g) ? r : g;
    if (b < x)
        x = b;
    v = (r > g) ? r : g;
    if (b > v)
        v = b;
          
    if (v == x) {
        hsv->h = 0;
        hsv->s = 0;
        hsv->v = v;
        return;
    }
       
    if (r == v) {
        f = g - b;
        i = 0;
    } else if (g == v) {
        f = b - r;
        i = 2 * 60;
    } else {
        f = r - g;
        i = 4 * 60;
    }
        
    hsv->h = i + (60 * f) / (v - x);
    if (hsv->h < 0)
        hsv->h += 360;

    hsv->s = (255 * (v - x)) / v;
    hsv->v = v;
         
    return;
}                                                                               

void Process(void *ctx, AVPicture *picture, enum PixelFormat pix_fmt, int width, int height, INT64 pts)
{
    ContextInfo *ci = (ContextInfo *) ctx;
    UINT8 *cm = cropTbl + MAX_NEG_CROP;                                         
    int rowsize = picture->linesize[0];

    if (pts < ci->next_pts)
        return;

    if (width < ci->min_width)
        return;

    ci->next_pts = pts + 1000000;    

    if (pix_fmt == PIX_FMT_YUV420P) {
        UINT8 *y, *u, *v;
        int width2 = width >> 1;
        int inrange = 0;
        int pixcnt;
        int h;
        int h_start, h_end;
        int w_start, w_end;

        h_end = 2 * ((ci->inset * height) / 200);
        h_start = height - h_end;

        w_end = (ci->inset * width2) / 100;
        w_start = width2 - w_end;

        pixcnt = ((h_start - h_end) >> 1) * (w_start - w_end);

        y = picture->data[0];
        u = picture->data[1];
        v = picture->data[2];

        for (h = h_start; h > h_end; h -= 2) {
            int w;

            for (w = w_start; w > w_end; w--) {
                int r,g,b;
                int Y, U, V;
                HSV hsv;

                U = u[0] - 128;
                V = v[0] - 128;

                Y = (y[0] - 16) * C_Y;

                r = cm[(Y + C_RV * V + (1 << (SCALE_BITS - 1))) >> SCALE_BITS];
                g = cm[(Y + - C_GU * U - C_GV * V + (1 << (SCALE_BITS - 1))) >> SCALE_BITS];
                b = cm[(Y + C_BU * U + (1 << (SCALE_BITS - 1))) >> SCALE_BITS];

                get_hsv(&hsv, r, g, b);

                if (ci->debug > 1) 
                    fprintf(stderr, "(%d,%d,%d) -> (%d,%d,%d)\n",
                        r,g,b,hsv.h,hsv.s,hsv.v);


                if (hsv.h >= ci->dark.h && hsv.h <= ci->bright.h &&
                    hsv.s >= ci->dark.s && hsv.s <= ci->bright.s &&
                    hsv.v >= ci->dark.v && hsv.v <= ci->bright.v) {            
                    inrange++;
                } else if (ci->zapping) {
                    y[0] = y[1] = y[rowsize] = y[rowsize + 1] = 0;
                }

                y+= 2;
                u++;
                v++;
            }

            y += picture->linesize[0] * 2 - width;
            u += picture->linesize[1] - width2;
            v += picture->linesize[2] - width2;
        }

        if (inrange * 1000 / pixcnt >= ci->threshold) {
            /* Save to file */
            int size;
            char *buf;
            AVPicture picture1;
            static int frame_counter;
            static int foundfile;

            if (ci->debug) 
                fprintf(stderr, "Fish: Inrange=%d of %d = %d threshold\n", inrange, pixcnt, 1000 * inrange / pixcnt);

            if ((frame_counter++ % 20) == 0) {
                /* Check how many files we have */
                DIR *d;

                foundfile = 0;

                d = opendir(ci->dir);
                if (d) {
                    struct dirent *dent;

                    while ((dent = readdir(d))) {
                        if (strncmp("fishimg", dent->d_name, 7) == 0) {
                            if (strcmp(".ppm", dent->d_name + strlen(dent->d_name) - 4) == 0) {
                                foundfile++;
                            }
                        }
                    }
                    closedir(d);
                }
            }

            if (foundfile < ci->file_limit) {
                size = avpicture_get_size(PIX_FMT_RGB24, width, height);
                buf = av_malloc(size);

                avpicture_fill(&picture1, buf, PIX_FMT_RGB24, width, height);
                if (img_convert(&picture1, PIX_FMT_RGB24, 
                                picture, pix_fmt, width, height) >= 0) {
                    /* Write out the PPM file */

                    FILE *f;
                    char fname[256];

                    sprintf(fname, "%s/fishimg%ld_%lld.ppm", ci->dir, time(0), pts);
                    f = fopen(fname, "w");
                    if (f) {
                        fprintf(f, "P6 %d %d 255\n", width, height);
                        fwrite(buf, width * height * 3, 1, f);
                        fclose(f);
                    }
                }

                av_free(buf);
                ci->next_pts = pts + ci->min_interval;    
            }
        }
    }
}

/* To ensure correct typing */
FrameHookConfigureFn ConfigureFn = Configure;
FrameHookProcessFn ProcessFn = Process;
