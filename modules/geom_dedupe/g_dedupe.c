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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/bio.h>
#include <sys/kernel.h>
#include <sys/limits.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/sbuf.h>
#include <geom/geom.h>

#include "g_dedupe.h"

#define G_DEDUPE_CLASS_NAME       "DEDUPE"
static MALLOC_DEFINE(M_GDEDUPE, "gdedupe data", "GEOM_DEDUPE Data");

SYSCTL_DECL(_kern_geom);
static SYSCTL_NODE(_kern_geom, OID_AUTO, dedupe, CTLFLAG_RW, 0,
    "GEOM_DEDUPE stuff");
static u_int g_dedupe_debug = 0;
SYSCTL_UINT(_kern_geom_dedupe, OID_AUTO, debug, CTLFLAG_RWTUN, &g_dedupe_debug, 0,
    "Debug level");
    
    
static int g_dedupe_destroy(struct g_geom *gp, boolean_t force);
static struct g_geom *g_dedupe_taste(struct g_class *mp, 
    struct g_provider *pp, int flags __unused);
    
static void g_dedupe_dumpconf(struct sbuf *sb, const char *indent,
    struct g_geom *gp, struct g_consumer *cp, struct g_provider *pp);
    
static void g_dedupe_init(struct g_class *mp);
static void g_dedupe_fini(struct g_class *mp);
static int g_dedupe_ioctl(struct g_provider *pp, u_long cmd, void *data,
    int fflag, struct thread *td);
    
static void g_dedupe_start(struct bio *bp);
static void g_dedupe_orphan(struct g_consumer *cp);
static int g_dedupe_access(struct g_provider *gp, int dr, int dw, int de);


/* Called right after class registration */
static void
g_dedupe_init(struct g_class *cp)
{
    G_DEDUPE_DEBUG(2, "Entering g_dedupe_init");
}

static void
g_dedupe_create(struct gctl_req *req, struct g_class *mp)
{
    struct g_dedupe_metadata md;
    struct g_dedupe_softc *sc;
    struct g_geom *gp;          /* This is us, an instance of a geom class */
    struct g_provider *newpp;   /* This is us, a provider of gdedupe */
    struct g_consumer *cp;      /* This is us, a consumer of the parent provider (pp) */
    struct g_provider *pp;          /* This is the parent provider */
    
    /* 1. Validate metadata in *md
     * 2. Create geom (gp)
     *  2.1 Create our softc (sc), configure, assign to geom (gp)
     * 3. Create provider (newpp)
     * 4. Create consumer (cp)
     * 5. Attach consumer to parent provider.
     */

    g_topology_assert();

    gp = NULL;
    newpp = NULL;
    cp = NULL;
    
    /* 1 */
    const char *name = gctl_get_asciiparam(req, "arg0");
    if(name == NULL) {
        gctl_error(req, "Invalid gdedupe name");
		return;
    }
    
    strlcpy(md.md_name, name, sizeof(md.md_name));
    strlcpy(md.md_magic, G_DEDUPE_MAGIC, sizeof(md.md_magic));
    md.md_version = G_DEDUPE_VERSION;
    
    /* Validate and find parent provider */
    name = gctl_get_asciiparam(req, "arg1");
        if(name == NULL) {
        gctl_error(req, "Invalid geom provider name");
		return;
    }
    
    pp = g_provider_by_name(name);
    if (pp == NULL) {
        gctl_error(req, "Unable to locate specified provider: %s", name);
        return;
    }

    G_DEDUPE_DEBUG(1, "Using geom provider: %s, secsize:%i, mediasize:%lu", 
        pp->name, pp->sectorsize, pp->mediasize);
        
    /* 2 */
    gp = g_new_geomf(mp, "%s", md.md_name);
    
    sc = g_malloc(sizeof(*sc), M_WAITOK | M_ZERO);
    mtx_init(&sc->sc_lock, "gdedupe lock", NULL, MTX_DEF);
    
    gp->softc = sc;
    gp->start = g_dedupe_start;
    gp->orphan = g_dedupe_orphan;
    gp->access = g_dedupe_access;
    gp->dumpconf = g_dedupe_dumpconf;
    
    /* 3 */
    newpp = g_new_providerf(gp, "dedupe/%s", gp->name);
    newpp->sectorsize = pp->sectorsize;
    newpp->mediasize = pp->mediasize;
    
    /* 4 */
    cp = g_new_consumer(gp);
    
    /* 5 */
    if(g_attach(cp, pp) != 0) {
        g_destroy_consumer(cp);
        g_destroy_provider(newpp);
        mtx_destroy(&sc->sc_lock);
        g_free(sc);
        g_destroy_geom(gp);
        return;
    }
    
    g_error_provider(newpp, 0);
    G_DEDUPE_DEBUG(1, "Device %s created.", gp->name);
    return;
}

static void
g_dedupe_start(struct bio *bp)
{
    
}

static void
g_dedupe_orphan(struct g_consumer *cp)
{
    g_topology_assert();

}

static int
g_dedupe_access(struct g_provider *pp, int dr, int dw, int de)
{
    struct g_geom *gp;
    struct g_consumer *cp;
    int error;
    
    gp = pp->geom;
    cp = LIST_FIRST(&gp->consumer);
    error = g_access(cp, dr, dw, de);
    
    return (error);
}

static void
g_dedupe_dumpconf(struct sbuf *sb, const char *indent, struct g_geom *gp,
    struct g_consumer *cp, struct g_provider *pp)
 {
     sbuf_printf(sb, "%s", "todo");
 }
/*
 * Pointer to function used for taste event handling.  If it
    is non-NULL it is called in three situations:

    -   On class activation, all existing providers are offered for tasting.
    -   When new provider is created it is offered for tasting.
    -   After last write access to a provider is closed it is offered for retasting (on first write open event ``spoil'' was sent).
*/
static struct g_geom *
g_dedupe_taste(struct g_class *mp, struct g_provider *pp,
               int flags __unused)
{
    g_topology_assert();
    
    return NULL;
}

static void
g_dedupe_config(struct gctl_req *req, struct g_class *cp,
   char const *verb)
{
    G_DEDUPE_DEBUG(1, "g_dedupe_ctlreq: verb: %s", verb);
    
    if(strcmp(verb, "create") == 0) {
            g_dedupe_create(req, cp);
            return;
    }
}

static int
g_dedupe_destroy_geom(struct gctl_req *req, struct g_class *cp,
   struct g_geom *gp)
{
    g_topology_assert();
        
    return 0;    
}
   
 static void
g_dedupe_fini(struct g_class *mp)
{
    
}

static struct g_class g_dedupe_class = {
  .name = G_DEDUPE_CLASS_NAME,
  .version = G_VERSION,
  .init = g_dedupe_init,
  .taste = g_dedupe_taste,
  .ctlreq = g_dedupe_config,
  .fini = g_dedupe_fini,
  .destroy_geom = g_dedupe_destroy_geom
};

DECLARE_GEOM_CLASS(g_dedupe_class, g_dedupe);
