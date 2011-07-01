REQUIREMENTS:
-------------
1) A Drupal 6 installation.
 - Apply the supplied database.inc.patc if you'd like to support concurrency.

2) Configure settings.php.
 - See the included settings.php.example for an example of configuring
   settings.php to use the dbn-write drush command.

CONFIGURATION:
--------------
See settings.php.example for a sample configuration that will write valid data
to the Drupal comments table.

The general configuration format is as follows:

  $config['dbn_write_tables'] = array(
    'TABLE_NAME' => array(
      'FIELD_NAME' => array(
        'callback' => 'FUNCTION',
        'arguments' => array('ARG1', 'ARG2', '...', 'ARGN'),
        'type' => 'text',
      ),
      'FIELD2_NAME' => array(
        'value' => 12345,
        'type' => 'int',
      ),
    ),
    'TABLE2_NAME' => ...
  );

Details:
 o 'callback' must point to a valid function.
 o 'arguments' is optional, define if your callback requires arguments.
 o 'type' can be 'text', 'int', or 'float'.
 o 'value' will be used if 'callback' is not defined.

Included callbacks:
 o dbn_random_string($length)         # includes funky characters
 o dbn_random_name($length)           # includes friendly characters
 o dbn_random_sentence($words)        # $words space-seperated random_names
 o dbn_random_paragraph($sentences)   # a paragraph of n sentences
 o dbn_random_paragraphs($paragraphs) # n paragraphs
 o dbn_random_nid()                   # a valid, active nid
 o dbn_random_uid()                   # a valid, active uid

EXAMPLES:
---------
$ drush ena dbn # enable the dbn module (to make the drush command available)

$ drush help dbn-write # see the inline help

$ drush dbnw --time=60 --child=10 # fork off 10 children all writing to the
                                  # database for 60 seconds.
