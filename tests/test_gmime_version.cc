# include <notmuch.h>
# include <gmime/gmime.h>

# include <iostream>

/* test if we are linked against the same GMime version as notmuch */

# if (GMIME_MAJOR_VERSION < 3)
# define g_mime_init() g_mime_init(0)
# define internet_address_list_parse(f,str) internet_address_list_parse_string(str)
# define internet_address_list_to_string(ia,f,encode) internet_address_list_to_string(ia,encode)
# endif 

int main () {
  g_mime_init ();

  const char * a = "Test <hey@asdf.da>";
  InternetAddressList * ia = internet_address_list_parse (NULL, a);

  const char * addrs = internet_address_list_to_string (ia, NULL, false);

  std::cout << "address: " << addrs << std::endl;

  return 0;
}

