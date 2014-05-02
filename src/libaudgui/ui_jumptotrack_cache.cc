/*
 * ui_jumptotrack_cache.c
 * Copyright 2008-2014 Jussi Judin and John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <glib.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/playlist.h>
#include <libaudcore/runtime.h>

#include "ui_jumptotrack_cache.h"

/**
 * Creates an regular expression list usable in searches from search keyword.
 *
 * In searches, every regular expression on this list is matched against
 * the search title and if they all match, the title is declared as
 * matching one.
 *
 * Regular expressions in list are formed by splitting the 'keyword' to words
 * by splitting the keyword string with space character.
 */
static GSList*
ui_jump_to_track_cache_regex_list_create(const char* keyword)
{
    GSList *regex_list = NULL;
    char **words = NULL;
    int i = -1;
    /* Chop the key string into ' '-separated key regex-pattern strings */
    words = g_strsplit(keyword, " ", 0);

    /* create a list of regex using the regex-pattern strings */
    while ( words[++i] != NULL )
    {
        // Ignore empty words.
        if (words[i][0] == 0) {
            continue;
        }

        GRegex * regex = g_regex_new (words[i], G_REGEX_CASELESS, (GRegexMatchFlags) 0, NULL);
        if (regex)
            regex_list = g_slist_append (regex_list, regex);
    }

    g_strfreev(words);

    return regex_list;
}

/**
 * Checks if 'song' matches all regular expressions in 'regex_list'.
 */
static bool_t
ui_jump_to_track_match(const char * song, GSList *regex_list)
{
    if ( song == NULL )
        return FALSE;

    for ( ; regex_list ; regex_list = g_slist_next(regex_list) )
    {
        GRegex * regex = (GRegex *) regex_list->data;
        if (! g_regex_match (regex, song, (GRegexMatchFlags) 0, NULL))
            return FALSE;
    }

    return TRUE;
}

/**
 * Returns all songs that match 'keyword'.
 *
 * Searches are conducted against entries in 'search_space' variable
 * and after the search, search result is added to 'cache'.
 *
 * @param cache The result of this search is added to cache.
 * @param search_space Entries inside which the search is conducted.
 * @param keyword Normalized string for searches.
 */
static const KeywordMatches *
ui_jump_to_track_cache_match_keyword(JumpToTrackCache* cache,
                                     const KeywordMatches* search_space,
                                     const char* keyword)
{
    GSList* regex_list = ui_jump_to_track_cache_regex_list_create(keyword);

    KeywordMatches * k = new KeywordMatches;

    for (auto & item : * search_space)
    {
        if (! regex_list ||
         ui_jump_to_track_match (item.title, regex_list) ||
         ui_jump_to_track_match (item.artist, regex_list) ||
         ui_jump_to_track_match (item.album, regex_list) ||
         ui_jump_to_track_match (item.path, regex_list))
            k->append (item);
    }

    g_hash_table_insert (cache, str_get (keyword), k);

    g_slist_free_full (regex_list, (GDestroyNotify) g_regex_unref);

    return k;
}

static void delete_matches (KeywordMatches * matches)
    { delete matches; }

/**
 * Creates a new song search cache.
 *
 * Returned value should be freed with ui_jump_to_track_cache_free() function.
 */
JumpToTrackCache*
ui_jump_to_track_cache_new()
{
    JumpToTrackCache * cache = g_hash_table_new_full (g_str_hash, g_str_equal,
     (GDestroyNotify) str_unref, (GDestroyNotify) delete_matches);

    // Initialize cache with playlist data
    int playlist = aud_playlist_get_active ();
    int entries = aud_playlist_entry_count (playlist);

    KeywordMatches * k = new KeywordMatches;

    for (int entry = 0; entry < entries; entry ++)
    {
        KeywordMatch & item = k->append ();

        item.entry = entry;
        aud_playlist_entry_describe (playlist, entry, item.title, item.artist, item.album, TRUE);
        item.path = uri_to_display (aud_playlist_entry_get_filename (playlist, entry));
    }

    // Finally insert all titles into cache into an empty key "" so that
    // the matchable data has specified place to be.
    g_hash_table_insert (cache, str_get (""), k);

    return cache;
}

/**
 * Searches 'keyword' inside 'playlist' by using 'cache' to speed up searching.
 *
 * Searches are basically conducted as follows:
 *
 * Cache is checked if it has the information about right playlist and
 * initialized with playlist data if needed.
 *
 * Keyword is normalized for searching (Unicode NFKD, case folding)
 *
 * Cache is checked if it has keyword and if it has, we can immediately get
 * the search results and return. If not, searching goes as follows:
 *
 * Search for the longest word that is in cache that matches the beginning
 * of keyword and use the cached matches as base for the current search.
 * The shortest word that can be matched against is the empty string "", so
 * there should always be matches in cache.
 *
 * After that conduct the search by splitting keyword into words separated
 * by space and using regular expressions.
 *
 * When the keyword is searched, search result is added to cache to
 * corresponding keyword that can be used as base for new searches.
 *
 * The motivation for caching is that to search word 'some cool song' one
 * has to type following strings that are all searched individually:
 *
 * s
 * so
 * som
 * some
 * some
 * some c
 * some co
 * some coo
 * some cool
 * some cool
 * some cool s
 * some cool so
 * some cool son
 * some cool song
 *
 * If the search results are cached in every phase and the result of
 * the maximum length matching string is used as base for concurrent
 * searches, we can probably get the matches reduced to some hundreds
 * after a few letters typed on playlists with thousands of songs and
 * reduce useless iteration quite a lot.
 *
 * Return: GArray of int
 */
const KeywordMatches * ui_jump_to_track_cache_search (JumpToTrackCache * cache,
 const char * keyword)
{
    GString* match_string = g_string_new(keyword);

    while (1)
    {
        const KeywordMatches * matches = (const KeywordMatches *)
         g_hash_table_lookup (cache, match_string->str);

        if (matches)
        {
            // if keyword matches something we have, we'll just return the list
            // of matches that the keyword has.
            if (! strcmp (match_string->str, keyword))
            {
                g_string_free(match_string, TRUE);
                return matches;
            }

            // Do normal search by using the result of previous search
            // as search space.
            matches = ui_jump_to_track_cache_match_keyword (cache, matches, keyword);

            g_string_free(match_string, TRUE);
            return matches;
        }

        if (! match_string->len)
            break;

        g_string_set_size (match_string, match_string->len - 1);
    }
    // This should never, ever get to this point because there is _always_
    // the empty string to match against.
    AUDDBG("One should never get to this point. Something is really wrong with \
cache->keywords hash table.");
    abort ();
}

void ui_jump_to_track_cache_free (JumpToTrackCache * cache)
{
    g_hash_table_unref (cache);
}
