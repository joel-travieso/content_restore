<?php
/**
 * @file
 * Administration area
 */

/**
 * Module's main page form.
 */
function content_restore_point_info_form($form, &$form_state) {
  // Build the sortable table header.
  $header = array(
    'title' => t('Title'),
    'type' => t('Type'),
    'bundle' => t('Bundle'),
    'id' => t('Entity Id'),
    'enabled' => t('Enabled'),
    'active_operations' => t('Active Operations'),
    'operations' => t('Operations'),
  );
  
  //Get restore points
  $result = db_select('content_restore_point', 'crp')
    ->fields('crp')
    ->execute()
    ->fetchAllAssoc('pid');

  //Build the rows.
  $options = array();
  foreach ($result as $pid => $data) {
    $op_map = array(
      'insert' => $data->on_insert,
      'update' => $data->on_update,
      'delete' => $data->on_delete,
    );
    $active_operations = array();
    foreach ($op_map as $op => $value) {
      if ($value) {
        $active_operations[] = $op;
      }
    }
    $active_operations = implode(', ', $active_operations);
    $info_form_path = current_path();
    $toggle_label = content_restore_point_enable_page_title(NULL, $data->enabled);
    $operations = array(
      l($toggle_label, "admin/structure/content_restore_point/manage/$pid/enable", array('query' => array('destination' => $info_form_path))),
      l(t('delete'), "admin/structure/content_restore_point/manage/$pid/delete", array('query' => array('destination' => $info_form_path))),
      l(t('restore'), "admin/structure/content_restore_point/manage/$pid/restore", array('query' => array('destination' => $info_form_path))),
    );
    $options[$pid] = array(
      'title' => array(
        'data' => array(
          '#type' => 'link',
          '#title' => $data->title,
          '#href' => 'admin/structure/content_restore_point/manage/' . $pid,
        ),
      ),
      'type' => $data->entity_type,
      'bundle' => $data->entity_bundle,
      'id' => $data->entity_id,
      'enabled' => $data->enabled ? t('Yes') : t('No'),
      'active_operations' => $active_operations,
      'operations' =>  implode(' | ', $operations),
    );
  }
  //Bulk operations
  $form['operation'] = array(
    '#type' => 'select',
    '#required' => TRUE,
    '#options' => array(
      'delete' => t('Delete selected items'),
      'toggle' => t('Toggle enabled status of the selected items'),
      'restore' => t('Restore selected items'),
    ),
  );
  $form['crp'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#empty' => t('No restore points available.'),
    '#required' => TRUE,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 100,
  );

  return $form;
}

/**
 * Submission handler for bulk operations in the main admin page.
 */
function content_restore_point_info_form_submit($form, &$form_state) {
  $pids = array();
  foreach ($form_state['values']['crp'] as $value) {
    if ($value) {
      $pids[] = $value;
    }
  }
  if (!empty($pids)) {
    module_load_include('inc', 'content_restore', 'content_restore_point.entity');
    switch ($form_state['values']['operation']) {
      case 'delete':
        content_restore_point_delete($pids);
        break;
      case 'toggle':
        foreach ($pids as $pid) {
          $enabled = $form_state['complete form']['crp']['#options'][$pid]['enabled'] == 'Yes' ? 0 : 1;
          content_restore_point_toggle($pid, $enabled);
        }
        break;
      case 'restore':
        content_restore_point_restore($pids);
        break;
    }
  }
}
