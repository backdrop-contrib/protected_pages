<?php
/**
 * @file
 * Provides page callbacks for configuration page.
 */

/**
 * Callback function for add protected page.
 */
function protected_pages_configure($form, &$form_state) {
  $form['help'] = array(
    '#type' => 'help',
    '#markup' => t('A password is required to view a protected page.'),
  );

  $form['rules_list'] = array(
    '#title' => t('Add Protected Page'),
    '#type' => 'fieldset',
  );

  $form = protected_page_form($form, $form_state);

  $form['rules_list']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Protect page'),
    '#attributes' => array('class' => array('button-primary')),
  );

  $form['pages_heading'] = array(
    '#type' => 'markup',
    '#markup' => '<h2>' . t('Protected Pages') . '</h2>',
  );
  $form['pages_table'] = array(
    '#markup' => protected_pages_get_pages_list(),
  );

  return $form;
}

/**
 * Callback to generate list of protected pages.
 */
function protected_pages_get_pages_list() {
  $header = array(
    array(
      'data' => t('#'),
      'width' => '10%',
    ),
    array(
      'data' => t('Path'),
      'width' => '60%',
    ),
    array(
      'data' => t('Operations'),
      'width' => '30%',
    ),
  );

  $rows = array();
  $query = db_select('protected_pages', 'pp')->extend('PagerDefault')->extend('TableSort');
  $query->fields('pp')
      ->limit(20)
      ->orderByHeader($header);

  $result = $query->execute();

  $rows = array();
  $count = 1;
  foreach ($result as $data) {
    $row = array();
    $row[] = array('data' => $count);
    $row[] = array('data' => l($data->path, url($data->path)));

    $links = array();
    $links['edit'] = array(
      'title' => t('Edit'),
      'href' => 'admin/config/system/protected_pages/' . $data->pid . '/edit',
    );
    $links['delete'] = array(
      'title' => t('Delete'),
      'href' => 'admin/config/system/protected_pages/' . $data->pid . '/delete',
    );
    $links['email'] = array(
      'title' => t('Send E-mail'),
      'href' => 'admin/config/system/protected_pages/' . $data->pid . '/send_email',
    );

    $operations = array(
      '#type' => 'operations',
      '#links' => $links,
    );

    $row[] = array('data' => $operations);

    $rows[] = $row;
    $count++;
  }

  $output = theme('table',
    array(
      'header' => $header,
      'rows' => $rows,
      'attributes' => array('width' => '500'),
      'sticky' => TRUE,
      'caption' => '',
      'colgroups' => array(),
      'empty' => t('No record found!'),
    ),
  );

  $output .= theme('pager', array('tags' => array()));

  return $output;
}

/**
 * Implements hook_validate().
 */
function protected_pages_configure_validate($form, &$form_state) {
  $form_state['normal_path'] = backdrop_get_normal_path($form_state['values']['path']);
  $path_alias = backdrop_strtolower(backdrop_get_path_alias($form_state['values']['path']));
  if (!backdrop_valid_path($form_state['normal_path'])) {
    form_set_error('path', t('Please enter a correct path!'));
  }

  $pid = db_select('protected_pages')
      ->fields('protected_pages', array('pid'))
      ->condition(db_or()->condition('path', $form_state['normal_path'])->condition('path', $path_alias))
      ->range(0, 1)
      ->execute()
      ->fetchField();
  if ($pid) {
    form_set_error('path', t('Duplicate path entry is not allowed. There is already a path or its alias exists.'));
  }
}

/**
 * Ajax submit callback for add protected page form.
 */
function protected_pages_configure_submit($form, &$form_state) {
  $path = check_plain($form_state['values']['path']);
  db_insert('protected_pages')->fields(array(
    'password' => sha1(trim(check_plain($form_state['values']['password']))),
    'path' => $path,
  ))->execute();

  backdrop_set_message(t('The page at :path has been successfully protected.', array(':path' => $path)));
}

/**
 * Callback function for edit protected page form.
 */
function protected_pages_edit($form, &$form_state, $pid) {
  $protected_page = db_select('protected_pages')
      ->fields('protected_pages', array('path'))
      ->condition('pid', $pid)
      ->execute()
      ->fetchObject();

  if (!isset($protected_page->path)) {
    backdrop_access_denied();
    exit;
  }
  $form['rules_list'] = array(
    '#title' => t('Protected Page settings'),
    '#type' => 'fieldset',
  );

  $form = protected_page_form($form, $form_state, $protected_page->path);

  $form['rules_list']['pid'] = array(
    '#type' => 'hidden',
    '#value' => $pid,
  );
  $form['rules_list']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * Helper function to add protected page elements.
 */
function protected_page_form($form, &$form_state, $path = NULL) {
  $form['rules_list']['path'] = array(
    '#type' => 'textfield',
    '#title' => t('Path'),
    '#size' => 30,
    '#field_prefix' => url(base_path(), array('absolute' => TRUE)),
    '#description' => t('Enter either a system path (<em>node/1</em>) or a URL alias (<em>about</em>).'),
    '#required' => TRUE,
    '#default_value' => $path,
  );
  $form['rules_list']['password'] = array(
    '#title' => t('Password'),
    '#type' => 'password',
    '#size' => 25,
    '#required' => TRUE,
  );

  return $form;
}

/**
 * Implements hook_validate().
 */
function protected_pages_edit_validate($form, &$form_state) {
  $form_state['normal_path'] = backdrop_get_normal_path($form_state['values']['path']);
  if (!backdrop_valid_path($form_state['normal_path'])) {
    form_set_error('path', t('Please enter a correct path!'));
  }

  $pid = db_select('protected_pages')
      ->fields('protected_pages', array('pid'))
      ->condition(db_or()->condition('path', $form_state['normal_path'])->condition('path', $form_state['values']['path']))
      ->condition('pid', $form_state['values']['pid'], '<>')
      ->range(0, 1)
      ->execute()
      ->fetchField();
  if ($pid) {
    form_set_error('path', t('Duplicate path entry is not allowed. There is already a path or its alias exists.'));
  }
}

/**
 * Implements hook_submit().
 */
function protected_pages_edit_submit($form, &$form_state) {

  $values['path'] = check_plain($form_state['values']['path']);
  if (!empty($form_state['values']['password'])) {
    $values['password'] = sha1(trim(check_plain($form_state['values']['password'])));
  }
  db_update('protected_pages')
      ->fields($values)
      ->condition('pid', $form_state['values']['pid'])
      ->execute();
  backdrop_set_message(t('The protected page settings have been successfully saved.'));
  $form_state['redirect'] = 'admin/config/system/protected_pages';
}

/**
 * Callback function for delete protected page.
 */
function protected_pages_delete_confirm($form, &$form_state, $pid) {

  $path = db_select('protected_pages')
      ->fields('protected_pages', array('path'))
      ->condition('pid', $pid)
      ->range(0, 1)
      ->execute()
      ->fetchField();

  $form['pid'] = array('#type' => 'hidden', '#value' => $pid);
  return confirm_form($form, t('Are you sure you want to delete <b>"%path"</b> from the protected pages list?', array('%path' => $path)), 'admin/config/system/protected_pages', t('This action cannot be undone.'), t('Delete'), t('Cancel')
  );
}

/**
 * Implements hook_submit().
 */
function protected_pages_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $pid = $form_state['values']['pid'];

    db_delete('protected_pages')
        ->condition('pid', $pid)
        ->execute();

    backdrop_set_message(t('The path has been successfully deleted from protected pages.'));
    $form_state['redirect'] = 'admin/config/system/protected_pages';
  }
}

/**
 * Callback function for send protected pages details email form.
 */
function protected_pages_send_email($form, &$form_state, $pid) {
  $config = config('protected_pages.settings');
  $destination = backdrop_get_destination();
  $form['send_email_box'] = array(
    '#type' => 'fieldset',
    '#title' => t('Send email'),
    '#description' => t('You send details of this protected page by email to multiple users. Please click !here to configure email settings.', array(
      '!here' => l(t('here'), 'admin/config/system/protected_pages/settings', array(
        'query' => $destination,
          )
      ),
        )
    ),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form_state['pid'] = $pid;

  $form['send_email_box']['recipents'] = array(
    '#type' => 'textarea',
    '#title' => t('Recipents'),
    '#rows' => 5,
    '#description' => t('Enter enter comma separated list of recipents.'),
    '#required' => TRUE,
  );
  $form['send_email_box']['subject'] = array(
    '#type' => 'textfield',
    '#title' => t('Subject'),
    '#default_value' => $config->get('protected_pages_email_subject'),
    '#description' => t('Enter subject of email.'),
    '#required' => TRUE,
  );
  $form['send_email_box']['body'] = array(
    '#type' => 'textarea',
    '#title' => t('Email Body'),
    '#rows' => 15,
    '#default_value' => $config->get('protected_pages_email_body'),
    '#description' => t('Enter the body of the email. Only [protected-page-url] and [site-name] tokens are available.
      Since password is encrypted, therefore we can not provide it by token.'),
    '#required' => TRUE,
  );

  $form['send_email_box']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Send Email'),
  );

  return $form;
}

/**
 * Implements hook_validate().
 */
function protected_pages_send_email_validate($form, &$form_state) {
  $emails = explode(',', str_replace(array("\r", "\n"), ',', $form_state['values']['recipents']));
  foreach ($emails as $key => $email) {
    $email = trim($email);
    if ($email) {
      if (!valid_email_address($email)) {
        form_error($form['send_email_box']['recipents'], t('Invalid email address: @mail. Please correct this email.', array('@mail' => $email)));
        unset($emails[$key]);
      }
      else {
        $emails[$key] = $email;
      }
    }
    else {
      unset($emails[$key]);
    }
  }
  $form_state['validated_recipents'] = implode(', ', $emails);
}

/**
 * Implements hook_submit().
 */
function protected_pages_send_email_submit($form, &$form_state) {
  $path = db_select('protected_pages')
      ->fields('protected_pages', array('path'))
      ->condition('pid', $form_state['pid'])
      ->range(0, 1)
      ->execute()
      ->fetchField();

  $module = 'protected_pages';
  $key = 'protected_pages_details_mail';
  $to = $form_state['values']['recipents'];
  $from = config_get('system.core', 'site_mail');
  $language = language_default();
  $send = TRUE;
  $params = array();
  $params['subject'] = $form_state['values']['subject'];
  $params['body'] = $form_state['values']['body'];
  $params['protected_page_url'] = url($path, array('absolute' => TRUE));

  backdrop_mail($module, $key, $to, $language, $params, $from, $send);
  backdrop_set_message(t('The email has been successfully sent.'));
  $form_state['redirect'] = 'admin/config/system/protected_pages';
}

/**
 * Callback function for protected pages settings.
 */
function protected_pages_settings() {
  $config = config('protected_pages.settings');

  $form['#config'] = 'protected_pages.settings';

  $form['protected_pages_password_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Password Settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['protected_pages_password_fieldset']['protected_pages_user_global_password'] = array(
    '#type' => 'radios',
    '#title' => t('Accepted passwords'),
    '#default_value' => $config->get('protected_pages_user_global_password'),
    '#options' => array(
      'per_page_or_global' => t('Global password or individual page password'),
      'per_page_password' => t('Only individual page password'),
      'only_global' => t('Only global password'),
    ),
    'per_page_or_global' => array(
      '#description' => t('Individual page passwords and global passwords will both be accepted.'),
    ),
    'per_page_password' => array(
      '#description' => t('Global password will not be accepted.'),
    ),
    'only_global' => array(
      '#description' => t('Individual page passwords will not be accepted.'),
    ),
  );
  $form['protected_pages_password_fieldset']['protected_pages_global_password_field'] = array(
    '#type' => 'password',
    '#title' => t('Global password'),
    '#description' => t('Necessary if you selected either "Global password or individual page password" or "Only global password" above.'),
  );
  $form['protected_pages_password_fieldset']['protected_pages_session_expire_time'] = array(
    '#type' => 'textfield',
    '#title' => t('Password expiration'),
    '#description' => t('After a password has been entered, a protected page will remain visible for this duration. After expiration, a password will need to be provided again. Enter 0 for an unlimited duration.'),
    '#default_value' => $config->get('protected_pages_session_expire_time'),
    '#required' => TRUE,
    '#size' => 10,
    '#element_validate' => array('protected_pages_validate_integer_positive'),
    '#field_suffix' => t('minutes'),
  );

  $form['protected_pages_email_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Email settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['protected_pages_email_fieldset']['protected_pages_email_subject'] = array(
    '#type' => 'textfield',
    '#title' => t('Email subject'),
    '#default_value' => $config->get('protected_pages_email_subject'),
  );
  $form['protected_pages_email_fieldset']['protected_pages_email_body'] = array(
    '#type' => 'textarea',
    '#title' => t('Email message'),
    '#rows' => 15,
    '#default_value' => $config->get('protected_pages_email_body'),
    '#description' => t('Only [protected-page-url] and [site-name] tokens are available.
      (Since the password is encrypted, it is not available as a token.)'),
  );

  $form['protected_pages_other_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Password Form Settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['protected_pages_other_fieldset']['protected_pages_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Page title'),
    '#default_value' => $config->get('protected_pages_title'),
    '#description' => t('Enter the title of the protected page.'),
  );
  $form['protected_pages_other_fieldset']['protected_pages_description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#default_value' => $config->get('protected_pages_description'),
    '#description' => t('This description is displayed inside the fieldset, above the password field. HTML is accepted.'),
  );
  $form['protected_pages_other_fieldset']['protected_pages_password_label'] = array(
    '#type' => 'textfield',
    '#title' => t('Password field label'),
    '#default_value' => $config->get('protected_pages_password_label'),
    '#description' => t('The label will appear above hte password field.'),
  );
  $form['protected_pages_other_fieldset']['protected_pages_submit_button_text'] = array(
    '#type' => 'textfield',
    '#title' => t('Submit button text'),
    '#default_value' => $config->get('protected_pages_submit_button_text'),
  );
  $form['protected_pages_other_fieldset']['protected_pages_incorrect_password_msg'] = array(
    '#type' => 'textarea',
    '#title' => t('Error text'),
    '#default_value' => $config->get('protected_pages_incorrect_password_msg'),
    '#description' => t('This error text will appear if someone provides an incorrect password.'),
  );

  $form['#submit'][] = '_protected_pages_settings_submit';

  return system_settings_form($form);
}

/**
 * Callback function to validate session expire time value.
 */
function protected_pages_validate_integer_positive($element, &$form_state) {
  $value = $element['#value'];
  if ($value !== '' && (!is_numeric($value) || intval($value) != $value || $value < 0)) {
    form_error($element, t('%name must be a positive integer.', array('%name' => $element['#title'])));
  }
}

/**
 * Custom submit function encrypt password and deleting non-useful variable.
 */
function _protected_pages_settings_submit($form, &$form_state) {
  $passwd = $form_state['values']['protected_pages_global_password_field'];
  if ($passwd) {
    $config = config('protected_pages.settings');
    $config->set('protected_pages_global_password', sha1($passwd));
    unset($form_state['values']['protected_pages_global_password_field']);
    $config->clear('protected_pages_global_password_field');
    $config->save();
  }
}
