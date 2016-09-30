/*-
 * Copyright (c) 2016 Tom Curry <thomasrcurry@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
 
static g_ctl_req_t g_dedupe_config;

struct g_dedupe_metadata {
          char            md_magic[16];
          uint32_t        md_version;
          char            md_name[16];
          uint64_t        md_provsize;
};
  

struct g_dedupe_softc {
        int             sc_error;
        off_t           sc_offset;
        off_t           sc_explicitsize;
        off_t           sc_stripesize;
        off_t           sc_stripeoffset;
        u_int           sc_rfailprob;
        u_int           sc_wfailprob;
        uintmax_t       sc_reads;
        uintmax_t       sc_writes;
        uintmax_t       sc_deletes;
        uintmax_t       sc_getattrs;
        uintmax_t       sc_flushes;
        uintmax_t       sc_cmd0s;
        uintmax_t       sc_cmd1s;
        uintmax_t       sc_cmd2s;
        uintmax_t       sc_readbytes;
        uintmax_t       sc_wrotebytes;
        struct mtx      sc_lock;
};
