<?php
/**
 * @file
 * CRP entity controller class.
 */
class ContentRestorePointController extends DrupalDefaultEntityController{
  public function save($content_restore_point) {
    drupal_write_record('content_restore_point', $content_restore_point);
    field_attach_insert('content_restore_point', $content_restore_point);
    module_invoke_all('entity_insert', 'content_restore_point', $content_restore_point);
    return $content_restore_point;
  }
}
