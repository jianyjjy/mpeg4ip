/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * mpeg4_file.cpp
 * Create media structures for session for an mp4v file (raw mpeg4)
 */
#include "player_session.h"
#include "player_media.h"
#include "our_bytestream_file.h"
#include "divx.h"
#include "divx_file.h"
#include <divxif.h>

static unsigned int c_get (void *ud)
{
  unsigned int ret;

  COurInByteStreamFile *fs = (COurInByteStreamFile *)ud;
  ret = fs->get() & 0xff;
  return (ret);
}

static void c_bookmark (void *ud, int val)
{
  COurInByteStreamFile *fs = (COurInByteStreamFile *)ud;
  fs->bookmark(val);
}

int create_media_for_divx_file (CPlayerSession *psptr, 
				const char *name,
				const char **errmsg)
{
  CPlayerMedia *mptr;
  COurInByteStreamFile *fbyte;
  int ret;
  juice_flag = 0;
  video_info_t *vid = NULL;

  fbyte = new COurInByteStreamFile(name);
  newdec_init(c_get, c_bookmark, fbyte);
#if 0
  ret = newdec_read_volvop();
#else
  do {
    try {
      ret = getvolhdr();
      if (ret == 1) {
	player_debug_message("Found vol in divx file");
      }
    } catch (int err) {
      ret = -1;
    }
  } while (ret == 0);
#endif
  delete fbyte;
  fbyte = NULL;
  if (ret <= 0) {
    vid = (video_info_t *)malloc(sizeof(video_info_t));
    vid->height = 240;
    vid->width = 320;
    vid->frame_rate = 30;
    vid->file_has_vol_header = 0;

  }

  psptr->session_set_seekable(0);
  mptr = new CPlayerMedia;
  if (mptr == NULL) {
    *errmsg = "Couldn't create media";
    return (-1);
  }

  fbyte = new COurInByteStreamFile(name);
  if (fbyte == NULL) {
    *errmsg = "Couldn't create file";
    return (-1);
  }

  fbyte->config_for_file(ret == 0 ? vid->frame_rate : mp4_hdr.fps);
  player_debug_message("Configuring for frame rate %d", 
		       ret == 0 ? vid->frame_rate : mp4_hdr.fps);
  *errmsg = "Couldn't create task";
  mptr->set_codec_type("divx");
  mptr->set_video_info(vid);

  return( mptr->create_from_file(psptr, fbyte, TRUE));
}
