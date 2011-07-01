<?php

/**
 * DataBase Noise.
 *
 * Drush script for generating database noise.  See settings.php.sample for
 * an example configuration that can be added to settings.php.
 */

/**
 * Tables to be written to should be configured in settings.php as follows:
 *
 * $config['dbn_write_tables'] = array(
 *   'TABLE_NAME' => array(
 *     'FIELD_NAME' => array(
 *       'callback' => 'FUNCTION',
 *       'options' => array('ARG1', 'ARG2', '...', 'ARGN'),
 *       'type' => 'text',
 *     ),
 *     'FIELD2_NAME' => array(
 *       'value' => 12345,
 *       'type' => 'int',
 *     ),
 *   ),
 *   'TABLE2_NAME' => ...
 * );
 *
 * Supported types:  int, float, text
 * Scroll down for a number of callback helpers.
 */

function dbn_drush_command() {
  $items = array();

  $items['dbn-write'] = array(
    'callback' => 'drush_dbn_write',
    'description' => 'Write to database.',
    'aliases' => array('dbnw'),
    'options' => array('--limit' => dt('Write LIMIT rows into all defined database tables (per thread)'), '--time' => dt('Write to database for TIME seconds'), '--child' => dt('Fork CHILD children processes.')),
    'examples' => array(
      'drush dbnw --limit=10' => 'Write 10 rows to the defined tables.',
      'drush dbnw --time=60 --child=5' => 'Write to the defined tables for 60 seconds.',
    ),
  );

  return $items;
}

/**
 * Writes to tables, forking children as necessary.
 */
function drush_dbn_write() {
  $verbose = drush_get_context('DRUSH_VERBOSE');
  $child = (int)drush_get_option('child');
  if ($child) {
    for ($children = 0; $children < $child; $children++) {
      $pid = pcntl_fork();
      if ($pid == -1) {
        die('Failed to fork.');
      }
      else if ($pid) {
        // we are the parent
        if ($children == ($child - 1)) {
          pcntl_wait($status); // Protect against Zombie children
        }
        db_set_active('default', TRUE);
      }
      else {
        db_set_active('default', TRUE);
        drush_dbn_write_sql();
        db_set_active('default', TRUE);
        return;
      }
    }
  }
  else {
    drush_dbn_write_sql();
  }
}

function drush_dbn_write_sql() {
  $verbose = drush_get_context('DRUSH_VERBOSE');
  if ($time = (int)drush_get_option('time')) {
    timer_start('dbn_write');
  }
  $limit = (int)drush_get_option('limit');

  $writes = variable_get('dbn_write_tables', array());

  $counter = 0;
  $loop = TRUE;
  while ($loop) {
    foreach ($writes as $table => $fields) {
      $sql = "INSERT INTO {$table} ";
      $field_array = $replace_array = $value_array = array();
      foreach ($fields as $field => $details) {
        $field_array[] = $field;
        if (isset($details['value'])) {
          $value = $details['value'];
        }
        else if (isset($details['callback']) && function_exists($details['callback'])) {
          if (isset($details['options'])) {
            $value = call_user_func_array($details['callback'], $details['options']);
          }
          else {
            $value = call_user_func($details['callback']);
          }
        }
        else {
          if (isset($details['callback'])) {
            drush_log(dt('Ignored undefined callback: !callback()', array('!callback' => $details['callback'])), 'error');
          }
          $value = dbn_random_name();
        }
        $value_array[] = $value;

        if (!isset($details['type'])) {
          $details['type'] = 'text';
        }
        switch($details['type']) {
          case 'int':
            $replace_array[] = '%d';
            break;
          case 'float':
            $replace_array[] = '%f';
            break;
          case 'text':
          default:
            $replace_array[] = "'%s'";
            break;
        }
      }
      $sql .= '('. implode(', ', $field_array) .') VALUES('. implode(', ', $replace_array) .')';
      if ($verbose){
        echo "$sql;\n";
      }
      db_query($sql, $value_array);
    }
    if ($time) {
      $elapsed = round(timer_read('dbn_write') / 1000);
      if ($elapsed > $time) {
        $loop = FALSE;
      }
    }
    $counter++;
    if ($limit && $counter >= $limit) {
      $loop = FALSE;
    }
  }
}

/**
 * Helper functions.
 */

/**
 * Generates a random string of ASCII characters of codes 32 to 126.
 *
 * @param $length
 *   Length of random string to generate.
 * @return
 *   Randomly generated string.
 */
function dbn_random_string($length = 8) {
  $str = '';
  for ($i = 0; $i < $length; $i++) {
    $str .= chr(mt_rand(32, 126));
  }
  return $str;
}

/**
 * Generates a random string containing letters and numbers.
 *
 * @param $length
 *   Length of random string to generate.
 * @return
 *   Randomly generated string.
 */
function dbn_random_name($length = 8) {
  $values = array_merge(range(65, 90), range(97, 122), range(48, 57));
  $max = count($values) - 1;
  $str = chr(mt_rand(97, 122));
  for ($i = 1; $i < $length; $i++) {
    $str .= chr($values[mt_rand(0, $max)]);
  }
  return $str;
}

/**
 * Generates a sentence of randomly generated noise words.
 */
function dbn_random_sentence($words = 16) {
  $str = dbn_random_name(rand(1,12));
  $words--;
  for ($i = 1; $i < $words; $i++) {
    $str .= ' '. dbn_random_name(rand(1,12));
  }
  return $str .'.';
}

/**
 * Generates a paragraph of randomly generated noise sentences.
 */
function dbn_random_paragraph($sentences = 5) {
  $str = dbn_random_sentence(rand(3,16));
  $sentences--;
  for ($i = 1; $i < $sentences; $i++) {
    $str .= ' '. dbn_random_sentence(rand(3,16));
  }
  return $str;
}

/**
 * Generates multiple paragraphs of randomly generated noise sentences.
 */
function dbn_random_paragraphs($paragraphs = 10) {
  $str = dbn_random_paragraph(rand(3,8));
  $paragraphs--;
  for ($i = 1; $i < $paragraphs; $i++) {
    $str .= "\n\n". dbn_random_paragraph(rand(3,8));
  }
  return $str;
}

/**
 * Returns a valid, active nid.
 */
function dbn_random_nid() {
  return db_result(db_query_range('SELECT nid FROM {node} WHERE status = 1 ORDER BY RAND()', 0, 1));
}

/**
 * Returns a valid, active uid.
 */
function dbn_random_uid() {
  return db_result(db_query_range('SELECT uid FROM {users} WHERE status = 1 ORDER BY RAND()', 0, 1));
}

