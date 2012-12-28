/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <string.h>
#include <zip.h>
#include <stdlib.h>
#include <assert.h>
#include <deadbeef/deadbeef.h>

#include <fex/fex.h>

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))

static DB_functions_t *deadbeef;
static DB_vfs_t plugin;

typedef struct {
    DB_FILE file;
    fex_t* fx;
    void* data;
    int64_t offset;
    fex_pos_t index;
    uint64_t size;
} fx_file_t;

static const char *scheme_names[] = { "fx://", NULL };

const char **
vfs_fx_get_schemes (void) {
    return scheme_names;
}

int
vfs_fx_is_streaming (void) {
    return 0;
}

// fname must have form of fx://full_filepath.ext:full_filepath_in_archive
DB_FILE*
vfs_fx_open (const char *fname) {
    if (strncasecmp (fname, "fx://", 5)) {
        return NULL;
    }

    fname += 5;
    const char *colon = strchr (fname, ':');
    if (!colon) {
        return NULL;
    }


    char fxname[colon-fname+1];
    memcpy (fxname, fname, colon-fname);
    fxname[colon-fname] = 0;

    fname = colon+1;

    fex_t * fx;

    if ( fex_open( &fx, fxname ) ) {
        return NULL;
    }

    while ( !fex_done( fx ) ) {
        if ( !strcmp( fex_name( fx ), fname ) ) break;
        fex_next( fx );
    }

    if ( fex_done( fx ) ) {
        fex_close( fx );
        return NULL;
    }

    fx_file_t *f = malloc (sizeof (fx_file_t));
    memset (f, 0, sizeof (fx_file_t));
    f->file.vfs = &plugin;
    f->fx = fx;
    f->index = fex_tell_arc( fx );
    fex_data( fx, &f->data );
    fex_stat( fx );
    f->size = fex_size( fx );
    return (DB_FILE*)f;
}

void
vfs_fx_close (DB_FILE *f) {
    fx_file_t *ff = (fx_file_t *)f;
    if (ff->fx) {
        fex_close( ff->fx );
    }
    free (ff);
}

size_t
vfs_fx_read (void *ptr, size_t size, size_t nmemb, DB_FILE *f) {
    fx_file_t *ff = (fx_file_t *)f;
    ssize_t rb = size * nmemb;
    if (rb > ff->size - ff->offset) rb = ff->size - ff->offset;
    memcpy(ptr, (const char*)ff->data + ff->offset, rb);
    ff->offset += rb;
    return rb / size;
}

int
vfs_fx_seek (DB_FILE *f, int64_t offset, int whence) {
    fx_file_t *ff = (fx_file_t *)f;

    if (whence == SEEK_CUR) {
        offset = ff->offset + offset;
    }
    else if (whence == SEEK_END) {
        offset = ff->size + offset;
    }

    if (offset <= ff->size) {
        ff->offset = offset;
    } else {
        ff->offset = ff->size;
    }

    return 0;
}

int64_t
vfs_fx_tell (DB_FILE *f) {
    fx_file_t *ff = (fx_file_t *)f;
    return ff->offset;
}

void
vfs_fx_rewind (DB_FILE *f) {
    fx_file_t *ff = (fx_file_t *)f;
    ff->offset = 0;
}

int64_t
vfs_fx_getlength (DB_FILE *f) {
    fx_file_t *ff = (fx_file_t *)f;
    return ff->size;
}

int
vfs_fx_scandir (const char *dir, struct dirent ***namelist, int (*selector) (const struct dirent *), int (*cmp) (const struct dirent **, const struct dirent **)) {
    fex_type_t *ft;

    if ( fex_identify_file( &ft, dir ) ) {
        return -1;
    }

    /* ignore binary pass-through files */
    if ( !strcmp( fex_type_name( ft ), "file" ) ) {
        return -1;
    }

    fex_t *fx;

    if ( fex_open_type( &fx, dir, ft ) ) {
        return -1;
    }

    int n = 0;
    while ( !fex_done( fx ) ) {
        ++n;
        fex_next( fx );
    }

    fex_rewind( fx );

    *namelist = malloc (sizeof (void *) * n);
    for (int i = 0; i < n; i++) {
        (*namelist)[i] = malloc (sizeof (struct dirent));
        memset ((*namelist)[i], 0, sizeof (struct dirent));
        const char *nm = fex_name( fx );
        snprintf ((*namelist)[i]->d_name, sizeof ((*namelist)[i]->d_name), "fx://%s:%s", dir, nm);
        fex_next( fx );
    }

    fex_close( fx );
    return n;
}

int
vfs_fx_is_container (const char *fname) {
    if (fex_identify_extension(fname)) {
        return 1;
    }
    return 0;
}

static DB_vfs_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_VFS,
    .plugin.id = "vfs_fx",
    .plugin.name = "File_Extractor vfs",
    .plugin.descr = "play files directly from zip/rar/gzip/7-zip files",
    .plugin.copyright = 
        "Copyright (C) 2012 Chris Moeller <kode54@gmail.com>\n"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
    ,
    .plugin.website = "http://github.com/kode54",
    .open = vfs_fx_open,
    .close = vfs_fx_close,
    .read = vfs_fx_read,
    .seek = vfs_fx_seek,
    .tell = vfs_fx_tell,
    .rewind = vfs_fx_rewind,
    .getlength = vfs_fx_getlength,
    .get_schemes = vfs_fx_get_schemes,
    .is_streaming = vfs_fx_is_streaming,
    .is_container = vfs_fx_is_container,
    .scandir = vfs_fx_scandir,
};

DB_plugin_t *
vfs_fx_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
