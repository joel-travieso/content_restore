<?php
/**
 * @file
 * CRP Entity utilities
 */

/**
 * Page callback for the CRP restore confirmation page.
 */
function content_restore_point_enable_confirm($form, &$form_state, $content_restore_point, $enable) {
  $action = content_restore_point_enable_page_title($content_restore_point);
  $form['pid'] = array('#type' => 'value', '#value' => $content_restore_point->pid);
  $form['enabled'] = array('#type' => 'value', '#value' => abs($content_restore_point->enabled - 1));
  return confirm_form($form,
    t("Are you sure you want to $action %title?", array('%title' => $content_restore_point->title)),
    'admin/structure/content_restore_point/manage/' . $content_restore_point->pid,
    t('You can change this back later'),
    ucfirst(t($action)),
    t('Cancel')
  );
}

/**
 * Page callback for the CRP deletion confirmation page.
 */
function content_restore_point_delete_confirm($form, &$form_state, $content_restore_point) {
  $form['pid'] = array('#type' => 'value', '#value' => $content_restore_point->pid);
  return confirm_form($form,
    t('Are you sure you want to delete %title?', array('%title' => $content_restore_point->title)),
    'admin/structure/content_restore_point/manage/' . $content_restore_point->pid,
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Page callback for the CRP restore confirmation page.
 */
function content_restore_point_restore_confirm($form, &$form_state, $content_restore_point) {
  $form['pid'] = array('#type' => 'value', '#value' => $content_restore_point->pid);
  return confirm_form($form,
    t('Are you sure you want to restore %title?', array('%title' => $content_restore_point->title)),
    'admin/structure/content_restore_point/manage/' . $content_restore_point->pid,
    t('This action cannot be undone.'),
    t('Restore'),
    t('Cancel')
  );
}

/**
 * Submission handler for the CRP restore confirmation page.
 */
function content_restore_point_enable_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    content_restore_point_toggle($form_state['values']['pid'], $form_state['values']['enabled']);
  }
  $form_state['redirect'] = content_restore_point_uri(NULL, $form_state['values']['pid']);
}

/**
 * Submission handler for the CRP deletion confirmation page.
 */
function content_restore_point_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    content_restore_point_delete($form_state['values']['pid']);
  }
  $form_state['redirect'] = 'admin/structure/content_restore_point';
}

/**
 * Submission handler for the CRP restore confirmation page.
 */
function content_restore_point_restore_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    content_restore_point_restore($form_state['values']['pid']);
    cache_clear_all();
  }
  $form_state['redirect'] = content_restore_point_uri(NULL, $form_state['values']['pid']);
}

/**
 * Enable/disable callback for CRP.
 */
function content_restore_point_toggle($pid, $enabled){
  db_update('content_restore_point')
    ->fields(array('enabled' => $enabled))
    ->condition('pid', $pid)
    ->execute();
}

/**
 * Deletion callback for CRP.
 */
function content_restore_point_delete($pid){
  $transaction = db_transaction();
  try {
    db_delete('content_restore_items')
      ->condition('pid', $pid)
      ->execute();
    db_delete('content_restore_point')
      ->condition('pid', $pid)
      ->execute();
  }
  catch (Exception $e) {
    $transaction->rollback();
    watchdog_exception('content_restore', $e);
    throw $e;
  }
}

/**
 * Restore callback for CRP.
 */
function content_restore_point_restore($pid){
  $result = db_select('content_restore_items', 'cri')
    ->fields('cri')
    ->condition('pid', $pid)
    ->execute();

  db_delete('content_restore_items')
    ->condition('pid', $pid)
    ->execute();

  //A flag to avoid changes part of this restoration process to be recursively stored as 'to restore' changes
  $_SESSION['content_restore_restoring'] = TRUE;

  while ($row = $result->fetchAssoc()) {
    switch ($row['operation']) {
      case 'insert':
        $insert_items[$row['entity_type']][] = $row['entity_id'];
        break;
      default:
        $item = unserialize($row['data']);
        entity_save($row['entity_type'], $item);
        break;
    }
  }

  if (!empty($insert_items)) {
    foreach ($insert_items as $entity_type => $ids) {
      entity_delete_multiple($entity_type, $ids);
    }
  }
  unset($_SESSION['content_restore_restoring']);
}

/**
 * Page callback for the 'Add CRP' page.
 */
function content_restore_point_add() {
  $content_restore_point = (object) array (
    'pid' => '',
    'type' => 'content_restore_point',
    'title' => '',
  );
  return drupal_get_form('content_restore_point_add_form', $content_restore_point);
}
 
/**
 * Form builder for the 'Add CRP' page.
 */
function content_restore_point_add_form($form, &$form_state, $content_restore_point) {
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#required' => TRUE,
    '#weight' => 1,
  );

  foreach (entity_get_info() as $key => $value) {
    $entity_types[$key] = $value['label'];
    $entity_type_bundles = array();
    foreach ($value['bundles'] as $bundle_key => $bundle_value) {
      $entity_type_bundles[$bundle_key] = $bundle_value['label'];
    }
    $form['bundles_' . $key] = array(
      '#type' => 'radios',
      '#options' => array(NULL => 'All') + $entity_type_bundles,
      '#title' => t($value['label'] . ' bundle'),
      '#weight' => 50,
      '#states' => array(
        'visible' => array(
         ':input[name="entity_type"]' => array('value' => $key),
        ),
      ),
    );
  }
  $form['entity_type'] = array(
    '#type' => 'radios',
    '#options' => array(NULL => 'All') + $entity_types,
    '#title' => t('Entity type'),
    '#weight' => 40,
  );
  $form['enabled'] = array(
    '#type' => 'checkbox', 
    '#title' => t('Enabled'),
    '#weight' => 2,
    '#default_value' => 1,
  );
  $form['on_insert'] = array(
    '#type' => 'checkbox', 
    '#title' => t('Act on insert'),
    '#weight' => 3,
  );
  $form['on_update'] = array(
    '#type' => 'checkbox', 
    '#title' => t('Act on update'),
    '#weight' => 3,
  );
  $form['on_delete'] = array(
    '#type' => 'checkbox', 
    '#title' => t('Act on delete'),
    '#weight' => 3,
    '#disabled' => TRUE,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 100,
  );

  field_attach_form('content_restore_point', $content_restore_point, $form, $form_state);

  return $form;
}

/**
 * Validation handler for the CRP add form.
 */
function content_restore_point_add_form_validate($form, &$form_state) {
  $content_restore_point_submission = (object) $form_state['values'];
  field_attach_form_validate('content_restore_point', $content_restore_point_submission, $form, $form_state);
}

/**
 * Submission handler for the CRP add form.
 */
function content_restore_point_add_form_submit($form, &$form_state) {
  $content_restore_point_submission = (object) $form_state['values'];
  if (empty($content_restore_point_submission->entity_type)) {
    unset($content_restore_point_submission->entity_type);
  }
  elseif (!empty($content_restore_point_submission->{'bundles_' . $content_restore_point_submission->entity_type})) {
    $content_restore_point_submission->entity_bundle = $content_restore_point_submission->{'bundles_' . $content_restore_point_submission->entity_type};
  }
  field_attach_submit('content_restore_point', $content_restore_point_submission, $form, $form_state);
  $content_restore_point = content_restore_point_save($content_restore_point_submission);
  $form_state['redirect'] = "admin/structure/content_restore_point/manage/$content_restore_point->pid";
}

/**
 * Save callback for CRP entity type.
 */
function content_restore_point_save(&$content_restore_point) {
  return entity_get_controller('content_restore_point')->save($content_restore_point);
}

/**
 * Page callback for individual CRP page.
 */
function content_restore_point_page_view($content_restore_point, $view_mode = 'full'){
  $content_restore_point->content = array();

  $header = array(
    'entity' => t('Entity'),
    'type' => t('Type'),
    'bundle' => t('Bundle'),
    'operation' => t('Operation'),
  );

  $result = db_select('content_restore_items', 'cri')
    ->fields('cri')
    ->condition('pid', $content_restore_point->pid)
    ->execute();

  $rows =  array();
  while ($row = $result->fetchAssoc()) {
    $entity = entity_load($row['entity_type'], array($row['entity_id']));
    $entity = array_shift($entity);

    $rows[] = array(
      l(entity_label($row['entity_type'], $entity), entity_uri($row['entity_type'], $entity)['path']),
      $row['entity_type'],
      $row['entity_bundle'],
      t($row['operation']),
    );
  }
  $table_element = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' =>t('Your table is empty'),
  );

  $content_restore_point->content['items'] = $table_element;
 
  // Build fields content.
  field_attach_prepare_view('content_restore_point', array($content_restore_point->pid => $content_restore_point), $view_mode);
  entity_prepare_view('content_restore_point', array($content_restore_point->pid => $content_restore_point));
  $content_restore_point->content += field_attach_view('content_restore_point', $content_restore_point, $view_mode);
 
  return $content_restore_point->content;
}
