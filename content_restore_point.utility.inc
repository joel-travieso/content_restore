<?php
/**
 * @file
 * Utility functions and callbacks
 */

/**
 * URI callback for CRP.
 */
function content_restore_point_uri($content_restore_point){
  return array(
    'path' => 'admin/structure/content_restore_point/manage/' . $content_restore_point->pid,
  );
}

/**
 * Load callback for CRP.
 */
function content_restore_point_load($pid = NULL, $reset = FALSE){
  $pids = (isset ($pid) ? array($pid) : array());
  $content_restore_point = content_restore_point_load_multiple($pids, $reset);
  return $content_restore_point ? reset ($content_restore_point) : FALSE;
}

/**
 * Multiple load callback for CRP.
 */
function content_restore_point_load_multiple($pids = array(), $conditions = array(), $reset = FALSE){
  return entity_load('content_restore_point', $pids, $conditions, $reset);
}

/**
 * Title callback for individual CRP page.
 */
function content_restore_point_page_title($content_restore_point){
  return $content_restore_point->title;
}

/**
 * Title callback for the enable/disable CRP page.
 */
function content_restore_point_enable_page_title($content_restore_point, $enabled = NULL){
  if ($content_restore_point) {
    return $content_restore_point->enabled ? t('disable') : t('enable');
  }
  elseif ($enabled !== NULL) {
    return $enabled ? t('disable') : t('enable');
  }
}

/**
 * Function to store changes done over content associated to corresponding CRPs in order to use it in restore later.
 */
function content_restore_store($entity, $type, $operation) {
  if (!empty($_SESSION['content_restore_restoring'])) {
    return;
  }
  if (is_string($entity)) {
    $entity_tmp = $entity;
    $entity = $type;
    $type = $entity_tmp;
  }
  $entity_info = entity_extract_ids($type, $entity);
  $type_or = db_or()->condition('entity_type', $type)->condition('entity_type', NULL);
  $bundle_or = db_or()->condition('entity_bundle', $entity_info[2])->condition('entity_bundle', NULL);
  $id_or = db_or()->condition('entity_id', $entity_info[0])->condition('entity_id', NULL);
  $result = db_select('content_restore_point', 'crp')
    ->fields('crp', array('pid'))
    ->condition('enabled', 1)
    ->condition('on_' . $operation, 1)
    ->condition($type_or)
    ->condition($bundle_or)
    ->condition($id_or)
    ->execute()
    ->fetchAllAssoc('pid');

  $data = $operation == 'update' ? $entity->original : ($operation == 'delete' ? $entity : NULL) ;
  foreach ($result as $pid => $value) {
    try {
      db_insert('content_restore_items')
        ->fields(array(
          'pid' => $pid,
          'entity_id' => $entity_info[0],
          'entity_type' => $type,
          'entity_bundle' => $entity_info[2],
          'operation' => $operation,
          'data' => serialize($data),
        ))
        ->execute();
    }
    catch (Exception $e) {}
  }
}
