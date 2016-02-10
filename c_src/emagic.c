#include <stdio.h>
#include <magic.h>
#include <string.h>
#include "erl_nif.h"

#define FLAG_FROM_FILE 0
#define FLAG_FROM_BUFFER 1

#define TYPE_MAGIC_MIME 0
#define TYPE_MAGIC_MIME_TYPE 1

// Ensure c strings are always null terminated
char * alloc_and_copy_to_cstring(ErlNifBinary *);

static ERL_NIF_TERM
magic_nif(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
    ErlNifBinary p;

     if (argc != 3)
    {
        return enif_make_badarg(env);
    }

    if (!enif_inspect_binary(env, argv[0], &p)) {
        return enif_make_badarg(env);
    }

    int flag = 0;
    if (!enif_get_int(env, argv[1], &flag))
    {
        return enif_make_badarg(env);
    }

    int type = 0;
    if (!enif_get_int(env, argv[2], &type))
    {
        return enif_make_badarg(env);
    }

    magic_t magic_cookie;
    //MAGIC_MIME tells magic to return a mime of the file, but you can specify different things
    switch(type){
        case TYPE_MAGIC_MIME:
            magic_cookie = magic_open(MAGIC_MIME);
            break;
        default:
            magic_cookie = magic_open(MAGIC_MIME_TYPE);
    }
    if (magic_cookie == NULL) {
        return enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "fail_to_open_magic"));
    }

     //Loading default magic database
    if (magic_load(magic_cookie, NULL) != 0) {
         // cannot load magic database
        magic_close(magic_cookie);
        return enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "fail_to_load_magic_database"));
    }

    const char *magic_full="";
    const char *path;
    switch(flag){
        case FLAG_FROM_FILE:
            path = alloc_and_copy_to_cstring(&p);
            magic_full = magic_file(magic_cookie, path);
            break;
        case FLAG_FROM_BUFFER:
            magic_full = magic_buffer(magic_cookie, p.data, p.size);
            break;
        default :
            magic_close(magic_cookie);
            return enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "not_support"));
    }

    enif_release_binary(&p);

    if(!magic_full){
        // Close magic database
        magic_close(magic_cookie);
        return enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "fail_to_magic"));
    }

    ErlNifBinary bin;
    size_t size = strlen(magic_full);
    if(!enif_alloc_binary(size, &bin))
    {
        // Close magic database
        magic_close(magic_cookie);
        return enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_atom(env, "fail_to_build_binary"));
    }
    memcpy(bin.data, magic_full, size);
    // Close magic database
    magic_close(magic_cookie);

    ERL_NIF_TERM value = enif_make_binary(env, &bin);

    return enif_make_tuple2(env, enif_make_atom(env, "ok"), value);
}

char * alloc_and_copy_to_cstring(ErlNifBinary *string)
{
  char *str = (char *) enif_alloc(string->size + 1);
  strncpy(str, (char *)string->data, string->size);
  str[string->size] = 0;
  return str;
}

static int
load(ErlNifEnv *env, void **priv, ERL_NIF_TERM info)
{

    return 0;
}

static int
reload(ErlNifEnv *env, void **priv, ERL_NIF_TERM info)
{
    return 0;
}

static int
upgrade(ErlNifEnv *env, void **priv, void **old_priv, ERL_NIF_TERM info)
{
    return load(env, priv, info);
}

static void
unload(ErlNifEnv *env, void *priv)
{
    return;
}

static ErlNifFunc nif_funcs[] = {
    {"magic", 3, magic_nif}
};

ERL_NIF_INIT(emagic, nif_funcs, &load, &reload, &upgrade, &unload);
