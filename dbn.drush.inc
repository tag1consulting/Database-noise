<?php

/**
 * DataBase Noise.
 *
 * Drush script for generating database noise.
 */

/**
 * Configured in settings.php, as follows:
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
 * );
 *
 * Supported types:  int, float, text
 */

function dbn_drush_command() {
  $items = array();

  $items['dbn-write'] = array(
    'callback' => 'drush_dbn_write',
    'description' => 'Write to database.',
    'aliases' => array('dbnw'),
  );

  return $items;
}

function drush_dbn_write() {
  $verbose = drush_get_context('DRUSH_VERBOSE');
  $child = (int)drush_get_option('child');
  if ($child) {
    for ($children = 0; $children < $child; $children++) {
      $pid = pcntl_fork();
      if ($pid == -1) {
        die('could not fork');
      }
      else if ($pid) {
        // we are the parent
        if ($children == ($child - 1)) {
          pcntl_wait($status); // Protect against Zombie children
        }
        db_set_active('default', TRUE);
        echo "Parent ($child) ($children) ($pid) done.\n";
      }
      else {
        db_set_active('default', TRUE);
        drush_dbn_write_sql();
        db_set_active('default', TRUE);
        echo "Child ($children) ($pid) done.\n";
        return;
      }
    }
  }
  else {
    drush_dbn_write_sql();
  }
}

function drush_dbn_write_sql() {
$config['dbn_write_tables'] = array(
  'queue' => array(
    'name' => array(
      'callback' => 'dbn_random_sentence',
      'options' => array(5),
      'type' => 'string',
    ),
    'data' => array(
      'callback' => 'dbn_random_paragraphs',
      'options' => array(50),
      'callback' => 'dbn_random_sentence',
      'options' => array(1),
      'type' => 'string',
    ),
    'expire' => array(
      'callback' => 'rand',
      'options' => array(10000000000, 99999999999),
      'type' => 'int',
    ),
    'created' => array(
      'callback' => 'rand',
      'options' => array(10000000000, 99999999999),
      'type' => 'int',
    ),
  ),
);
  $verbose = drush_get_context('DRUSH_VERBOSE');
  if ($time = (int)drush_get_option('time')) {
    timer_start('dbn_write');
  }
  $limit = (int)drush_get_option('limit');

  $writes = variable_get('dbn_write_tables', array());
  $writes = $config['dbn_write_tables'];

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
