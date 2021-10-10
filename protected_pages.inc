<?php
/**
 * @file
 * Provides page callbacks for enter password page.
 */

/**
 * Callback function for enter password page.
 */
function protected_pages_enter_password($form, &$form_state) {
  $config = config('protected_pages.settings');
  if (!isset($_GET['destination']) || empty($_GET['protected_page']) || !is_numeric($_GET['protected_page'])) {
    watchdog('protected_page', 'Illegal call to /protected-page', array(), WATCHDOG_WARNING);
    backdrop_access_denied();
    exit();
  }
  $pid = db_select('protected_pages')
      ->fields('protected_pages', array('pid'))
      ->condition('pid', $_GET['protected_page'])
      ->range(0, 1)
      ->execute()
      ->fetchField();
  if (!$pid) {
    watchdog('protected_page', 'Invalid pid (@pid) used with /protected-page', array('@pid' => $_GET['protected_page']), WATCHDOG_WARNING);
    backdrop_access_denied();
    exit();
  }
  $form['protected_page_enter_password'] = array(
    '#type' => 'fieldset',
    '#description' => $config->get('protected_pages_description'),
    '#collapsible' => FALSE,
  );

  $form['protected_page_enter_password']['password'] = array(
    '#type' => 'password',
    '#title' => $config->get('protected_pages_password_label'),
    '#size' => 20,
    '#required' => TRUE,
  );

  $form['protected_page_pid'] = array(
    '#type' => 'hidden',
    '#value' => $_GET['protected_page'],
  );

  $form['protected_page_enter_password']['submit'] = array(
    '#type' => 'submit',
    '#value' => $config->get('protected_pages_submit_button_text'),
  );

  return $form;
}

/**
 * Implements hook_validate().
 */
function protected_pages_enter_password_validate($form, &$form_state) {
  $config = config('protected_pages.settings');

  $password = sha1(trim(check_plain($form_state['values']['password'])));
  $global_password_setting = $config->get('protected_pages_user_global_password');
  if ($global_password_setting == 'per_page_password') {
    $pid = db_select('protected_pages')
        ->fields('protected_pages', array('pid'))
        ->condition('password', $password)
        ->condition('pid', $form_state['values']['protected_page_pid'])
        ->range(0, 1)
        ->execute()
        ->fetchField();
    if (!$pid) {

      form_set_error('password', $config->get('protected_pages_incorrect_password_msg'));
    }
  }
  elseif ($global_password_setting == 'per_page_or_global') {
    $pid = db_select('protected_pages')
        ->fields('protected_pages', array('pid'))
        ->condition('password', $password)
        ->condition('pid', $form_state['values']['protected_page_pid'])
        ->range(0, 1)
        ->execute()
        ->fetchField();

    $global_password = $config->get('protected_pages_global_password');
    if (!$pid && $global_password != $password) {
      form_set_error('password', $config->get('protected_pages_incorrect_password_msg'));
    }
  }
  else {
    $global_password = $config->get('protected_pages_global_password');
    if ($global_password != $password) {
      form_set_error('password', $config->get('protected_pages_incorrect_password_msg'));
    }
  }
}

/**
 * Implements hook_submit().
 */
function protected_pages_enter_password_submit($form, &$form_state) {
  $_SESSION['_protected_page']['passwords'][$form_state['values']['protected_page_pid']]['request_time'] = REQUEST_TIME;
  $session_expire_time = config_get('protected_pages.settings', 'protected_pages_session_expire_time');
  if ($session_expire_time) {
    $_SESSION['_protected_page']['passwords'][$form_state['values']['protected_page_pid']]['expire_time'] = strtotime("+{$session_expire_time} minutes");
  }
}
