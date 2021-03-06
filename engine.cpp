// $Id$

#include "main.h"
#include "ssdeep.h"
#include "match.h"

#define MAX_STATUS_MSG   78

bool display_result(state *s, const TCHAR * fn, const char * sum) {
  // Only spend the extra time to make a Filedata object if we need to
  if (MODE(mode_match_pretty) || MODE(mode_match) || MODE(mode_directory)) {
    Filedata * f = NULL;
    
    try {
      f = new Filedata(fn, sum);
    }
    catch (std::bad_alloc) {
      fatal_error("%s: Unable to create Filedata object in engine.cpp:display_result()", __progname);
    }

    if (MODE(mode_match_pretty)) {
      if (match_add(s, f))
	print_error_unicode(s, fn, "Unable to add hash to set of known hashes");
    }
    else {
      // This block is for MODE(mode_match) || MODE(mode_directory)
      match_compare(s, f);
      
      if (MODE(mode_directory)) {
	if (match_add(s, f))
	  print_error_unicode(s,
			      fn,
			      "Unable to add hash to set of known hashes");
      } else {
	// We haven't add f to the set of knowns, so let's free it.
	delete f;
      }
    }
  }
  else
  {
    // No special options selected. Display the hash for this file
    if (s->first_file_processed)
    {
      print_status("%s", OUTPUT_FILE_HEADER);
      s->first_file_processed = false;
    }

    printf ("%s,\"", sum);
    display_filename(stdout, fn, TRUE);
    print_status("\"");
  }

  return false;
}


int hash_file(state *s, TCHAR *fn) {
  size_t fn_length;
  char *sum;
  TCHAR *my_filename, *msg;
  FILE *handle;

#ifdef WIN32
  TCHAR expanded_fn[SSDEEP_PATH_MAX];
  if (!expanded_path(fn) && !(s->mode & mode_relative)) {
    _sntprintf(expanded_fn, 
	       SSDEEP_PATH_MAX,
	       _TEXT("\\\\?\\%s"),
	       fn);
  } else {
    _tcsncpy(expanded_fn, fn, SSDEEP_PATH_MAX);
  }
  handle = _tfopen(expanded_fn, _TEXT("rb"));
# else
  handle = fopen(fn, "rb");
#endif

  if (NULL == handle)
  {
    print_error_unicode(s,fn,"%s", strerror(errno));
    return TRUE;
  }
 
  if ((sum = (char *)malloc(sizeof(char) * FUZZY_MAX_RESULT)) == NULL)
  {
    fclose(handle);
    print_error_unicode(s,fn,"%s", strerror(errno));
    return TRUE;
  }

#define MSG_LENGTH (MAX_STATUS_MSG + 20)
  if ((msg = (TCHAR *)malloc(sizeof(TCHAR) * MSG_LENGTH)) == NULL)
  {
    free(sum);
    fclose(handle);
    print_error_unicode(s,fn,"%s", strerror(errno));
    return TRUE;
  }

  if (MODE(mode_verbose))
  {
    fn_length = _tcslen(fn);
    if (fn_length > MAX_STATUS_MSG)
    {
      // We have to make a duplicate of the string to call basename on it
      // We need the original name for the output later on
      my_filename = _tcsdup(fn);
      my_basename(my_filename);
    }
    else
      my_filename = fn;

    _sntprintf(msg,
	       MSG_LENGTH-1,
	       _TEXT("Hashing: %s%s"),
	       my_filename,
	       _TEXT(BLANK_LINE));
    _ftprintf(stderr,_TEXT("%s\r"), msg);

    if (fn_length > MAX_STATUS_MSG)
      free(my_filename);
  }

  fuzzy_hash_file(handle,sum);
  prepare_filename(s,fn);
  display_result(s,fn,sum);

  if (find_file_size(handle) > SSDEEP_MIN_FILE_SIZE)
    s->found_meaningful_file = true;
  s->processed_file = true;

  fclose(handle);
  free(sum);
  free(msg);
  return FALSE;
}

