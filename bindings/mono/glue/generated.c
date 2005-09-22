// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

#include <libredcarpet.h>

guint rcsharp_rc_package_get_installed_offset (void);

guint
rcsharp_rc_package_get_installed_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCPackage, installed);
}

guint rcsharp_rc_package_get_local_package_offset (void);

guint
rcsharp_rc_package_get_local_package_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCPackage, local_package);
}

guint rcsharp_rc_package_get_package_set_offset (void);

guint
rcsharp_rc_package_get_package_set_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCPackage, package_set);
}

guint rcsharp_rc_package_get_history_offset (void);

guint
rcsharp_rc_package_get_history_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCPackage, history);
}

guint rcsharp_rc_packagefile_get_mode_offset (void);

guint
rcsharp_rc_packagefile_get_mode_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCPackageFile, mode);
}

guint rcsharp_rc_packagefile_get_md5sum_offset (void);

guint
rcsharp_rc_packagefile_get_md5sum_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCPackageFile, md5sum);
}

guint rcsharp_rc_packagefile_get_mtime_offset (void);

guint
rcsharp_rc_packagefile_get_mtime_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCPackageFile, mtime);
}

guint rcsharp_rc_packagefile_get_ghost_offset (void);

guint
rcsharp_rc_packagefile_get_ghost_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCPackageFile, ghost);
}

guint rcsharp_rc_packagefile_get_uid_offset (void);

guint
rcsharp_rc_packagefile_get_uid_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCPackageFile, uid);
}

guint rcsharp_rc_packagefile_get_link_target_offset (void);

guint
rcsharp_rc_packagefile_get_link_target_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCPackageFile, link_target);
}

guint rcsharp_rc_packagefile_get_filename_offset (void);

guint
rcsharp_rc_packagefile_get_filename_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCPackageFile, filename);
}

guint rcsharp_rc_packagefile_get_size_offset (void);

guint
rcsharp_rc_packagefile_get_size_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCPackageFile, size);
}

guint rcsharp_rc_packagefile_get_gid_offset (void);

guint
rcsharp_rc_packagefile_get_gid_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCPackageFile, gid);
}

guint rcsharp_rc_packageupdate_get_package_offset (void);

guint
rcsharp_rc_packageupdate_get_package_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCPackageUpdate, package);
}

guint rcsharp_rc_queueitem_get_world_offset (void);

guint
rcsharp_rc_queueitem_get_world_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCQueueItem, world);
}

guint rcsharp_rc_queueitem_get_priority_offset (void);

guint
rcsharp_rc_queueitem_get_priority_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCQueueItem, priority);
}

guint rcsharp_rc_queueitem_get_pending_info_offset (void);

guint
rcsharp_rc_queueitem_get_pending_info_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCQueueItem, pending_info);
}

guint rcsharp_rc_queueitem_get_type_offset (void);

guint
rcsharp_rc_queueitem_get_type_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCQueueItem, type);
}

guint rcsharp_rc_queueitem_get_size_offset (void);

guint
rcsharp_rc_queueitem_get_size_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCQueueItem, size);
}

gboolean rcsharp_rc_queueitem_conflict_get_actually_an_obsolete (RCQueueItem_Conflict *queueitem_conflict);
void rcsharp_rc_queueitem_conflict_set_actually_an_obsolete (RCQueueItem_Conflict *queueitem_conflict, gboolean value);

gboolean
rcsharp_rc_queueitem_conflict_get_actually_an_obsolete (RCQueueItem_Conflict *queueitem_conflict)
{
	return (gboolean)queueitem_conflict->actually_an_obsolete;
}

void
rcsharp_rc_queueitem_conflict_set_actually_an_obsolete (RCQueueItem_Conflict *queueitem_conflict, gboolean value)
{
	queueitem_conflict->actually_an_obsolete = (gboolean)value;
}

gboolean rcsharp_rc_queueitem_install_get_explicitly_requested (RCQueueItem_Install *queueitem_install);
void rcsharp_rc_queueitem_install_set_explicitly_requested (RCQueueItem_Install *queueitem_install, gboolean value);

gboolean
rcsharp_rc_queueitem_install_get_explicitly_requested (RCQueueItem_Install *queueitem_install)
{
	return (gboolean)queueitem_install->explicitly_requested;
}

void
rcsharp_rc_queueitem_install_set_explicitly_requested (RCQueueItem_Install *queueitem_install, gboolean value)
{
	queueitem_install->explicitly_requested = (gboolean)value;
}

gboolean rcsharp_rc_queueitem_require_get_remove_only (RCQueueItem_Require *queueitem_require);
void rcsharp_rc_queueitem_require_set_remove_only (RCQueueItem_Require *queueitem_require, gboolean value);

gboolean
rcsharp_rc_queueitem_require_get_remove_only (RCQueueItem_Require *queueitem_require)
{
	return (gboolean)queueitem_require->remove_only;
}

void
rcsharp_rc_queueitem_require_set_remove_only (RCQueueItem_Require *queueitem_require, gboolean value)
{
	queueitem_require->remove_only = (gboolean)value;
}

gboolean rcsharp_rc_queueitem_uninstall_get_explicitly_requested (RCQueueItem_Uninstall *queueitem_uninstall);
void rcsharp_rc_queueitem_uninstall_set_explicitly_requested (RCQueueItem_Uninstall *queueitem_uninstall, gboolean value);

gboolean
rcsharp_rc_queueitem_uninstall_get_explicitly_requested (RCQueueItem_Uninstall *queueitem_uninstall)
{
	return (gboolean)queueitem_uninstall->explicitly_requested;
}

void
rcsharp_rc_queueitem_uninstall_set_explicitly_requested (RCQueueItem_Uninstall *queueitem_uninstall, gboolean value)
{
	queueitem_uninstall->explicitly_requested = (gboolean)value;
}

gboolean rcsharp_rc_queueitem_uninstall_get_remove_only (RCQueueItem_Uninstall *queueitem_uninstall);
void rcsharp_rc_queueitem_uninstall_set_remove_only (RCQueueItem_Uninstall *queueitem_uninstall, gboolean value);

gboolean
rcsharp_rc_queueitem_uninstall_get_remove_only (RCQueueItem_Uninstall *queueitem_uninstall)
{
	return (gboolean)queueitem_uninstall->remove_only;
}

void
rcsharp_rc_queueitem_uninstall_set_remove_only (RCQueueItem_Uninstall *queueitem_uninstall, gboolean value)
{
	queueitem_uninstall->remove_only = (gboolean)value;
}

gboolean rcsharp_rc_queueitem_uninstall_get_due_to_conflict (RCQueueItem_Uninstall *queueitem_uninstall);
void rcsharp_rc_queueitem_uninstall_set_due_to_conflict (RCQueueItem_Uninstall *queueitem_uninstall, gboolean value);

gboolean
rcsharp_rc_queueitem_uninstall_get_due_to_conflict (RCQueueItem_Uninstall *queueitem_uninstall)
{
	return (gboolean)queueitem_uninstall->due_to_conflict;
}

void
rcsharp_rc_queueitem_uninstall_set_due_to_conflict (RCQueueItem_Uninstall *queueitem_uninstall, gboolean value)
{
	queueitem_uninstall->due_to_conflict = (gboolean)value;
}

gboolean rcsharp_rc_queueitem_uninstall_get_due_to_obsolete (RCQueueItem_Uninstall *queueitem_uninstall);
void rcsharp_rc_queueitem_uninstall_set_due_to_obsolete (RCQueueItem_Uninstall *queueitem_uninstall, gboolean value);

gboolean
rcsharp_rc_queueitem_uninstall_get_due_to_obsolete (RCQueueItem_Uninstall *queueitem_uninstall)
{
	return (gboolean)queueitem_uninstall->due_to_obsolete;
}

void
rcsharp_rc_queueitem_uninstall_set_due_to_obsolete (RCQueueItem_Uninstall *queueitem_uninstall, gboolean value)
{
	queueitem_uninstall->due_to_obsolete = (gboolean)value;
}

gboolean rcsharp_rc_queueitem_uninstall_get_unlink (RCQueueItem_Uninstall *queueitem_uninstall);
void rcsharp_rc_queueitem_uninstall_set_unlink (RCQueueItem_Uninstall *queueitem_uninstall, gboolean value);

gboolean
rcsharp_rc_queueitem_uninstall_get_unlink (RCQueueItem_Uninstall *queueitem_uninstall)
{
	return (gboolean)queueitem_uninstall->unlink;
}

void
rcsharp_rc_queueitem_uninstall_set_unlink (RCQueueItem_Uninstall *queueitem_uninstall, gboolean value)
{
	queueitem_uninstall->unlink = (gboolean)value;
}

guint rcsharp_rc_resolver_get_current_channel_offset (void);

guint
rcsharp_rc_resolver_get_current_channel_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolver, current_channel);
}

guint rcsharp_rc_resolver_get_timed_out_offset (void);

guint
rcsharp_rc_resolver_get_timed_out_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolver, timed_out);
}

guint rcsharp_rc_resolver_get_packages_to_install_offset (void);

guint
rcsharp_rc_resolver_get_packages_to_install_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolver, packages_to_install);
}

guint rcsharp_rc_resolver_get_verifying_offset (void);

guint
rcsharp_rc_resolver_get_verifying_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolver, verifying);
}

guint rcsharp_rc_resolver_get_packages_to_remove_offset (void);

guint
rcsharp_rc_resolver_get_packages_to_remove_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolver, packages_to_remove);
}

guint rcsharp_rc_resolver_get_extra_deps_offset (void);

guint
rcsharp_rc_resolver_get_extra_deps_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolver, extra_deps);
}

guint rcsharp_rc_resolver_get_pruned_queues_offset (void);

guint
rcsharp_rc_resolver_get_pruned_queues_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolver, pruned_queues);
}

guint rcsharp_rc_resolver_get_valid_solution_count_offset (void);

guint
rcsharp_rc_resolver_get_valid_solution_count_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolver, valid_solution_count);
}

guint rcsharp_rc_resolver_get_extra_conflicts_offset (void);

guint
rcsharp_rc_resolver_get_extra_conflicts_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolver, extra_conflicts);
}

guint rcsharp_rc_resolver_get_timeout_seconds_offset (void);

guint
rcsharp_rc_resolver_get_timeout_seconds_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolver, timeout_seconds);
}

guint rcsharp_rc_resolver_get_deferred_queues_offset (void);

guint
rcsharp_rc_resolver_get_deferred_queues_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolver, deferred_queues);
}

guint rcsharp_rc_resolver_get_pending_queues_offset (void);

guint
rcsharp_rc_resolver_get_pending_queues_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolver, pending_queues);
}

guint rcsharp_rc_resolver_get_packages_to_verify_offset (void);

guint
rcsharp_rc_resolver_get_packages_to_verify_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolver, packages_to_verify);
}

guint rcsharp_rc_resolver_get_complete_queues_offset (void);

guint
rcsharp_rc_resolver_get_complete_queues_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolver, complete_queues);
}

guint rcsharp_rc_resolver_get_initial_items_offset (void);

guint
rcsharp_rc_resolver_get_initial_items_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolver, initial_items);
}

guint rcsharp_rc_resolvercontext_get_last_checked_status_offset (void);

guint
rcsharp_rc_resolvercontext_get_last_checked_status_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolverContext, last_checked_status);
}

guint rcsharp_rc_resolvercontext_get_parent_offset (void);

guint
rcsharp_rc_resolvercontext_get_parent_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolverContext, parent);
}

guint rcsharp_rc_resolvercontext_get_refs_offset (void);

guint
rcsharp_rc_resolvercontext_get_refs_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolverContext, refs);
}

gboolean rcsharp_rc_resolvercontext_get_verifying (RCResolverContext *resolvercontext);
void rcsharp_rc_resolvercontext_set_verifying (RCResolverContext *resolvercontext, gboolean value);

gboolean
rcsharp_rc_resolvercontext_get_verifying (RCResolverContext *resolvercontext)
{
	return (gboolean)resolvercontext->verifying;
}

void
rcsharp_rc_resolvercontext_set_verifying (RCResolverContext *resolvercontext, gboolean value)
{
	resolvercontext->verifying = (gboolean)value;
}

guint rcsharp_rc_resolvercontext_get_last_checked_package_offset (void);

guint
rcsharp_rc_resolvercontext_get_last_checked_package_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolverContext, last_checked_package);
}

guint rcsharp_rc_resolvercontext_get_download_size_offset (void);

guint
rcsharp_rc_resolvercontext_get_download_size_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolverContext, download_size);
}

guint rcsharp_rc_resolvercontext_get_max_priority_offset (void);

guint
rcsharp_rc_resolvercontext_get_max_priority_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolverContext, max_priority);
}

guint rcsharp_rc_resolvercontext_get_install_size_offset (void);

guint
rcsharp_rc_resolvercontext_get_install_size_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolverContext, install_size);
}

gboolean rcsharp_rc_resolvercontext_get_invalid (RCResolverContext *resolvercontext);
void rcsharp_rc_resolvercontext_set_invalid (RCResolverContext *resolvercontext, gboolean value);

gboolean
rcsharp_rc_resolvercontext_get_invalid (RCResolverContext *resolvercontext)
{
	return (gboolean)resolvercontext->invalid;
}

void
rcsharp_rc_resolvercontext_set_invalid (RCResolverContext *resolvercontext, gboolean value)
{
	resolvercontext->invalid = (gboolean)value;
}

guint rcsharp_rc_resolvercontext_get_world_offset (void);

guint
rcsharp_rc_resolvercontext_get_world_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolverContext, world);
}

guint rcsharp_rc_resolvercontext_get_total_priority_offset (void);

guint
rcsharp_rc_resolvercontext_get_total_priority_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolverContext, total_priority);
}

guint rcsharp_rc_resolvercontext_get_log_offset (void);

guint
rcsharp_rc_resolvercontext_get_log_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolverContext, log);
}

guint rcsharp_rc_resolvercontext_get_other_penalties_offset (void);

guint
rcsharp_rc_resolvercontext_get_other_penalties_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolverContext, other_penalties);
}

guint rcsharp_rc_resolvercontext_get_status_offset (void);

guint
rcsharp_rc_resolvercontext_get_status_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolverContext, status);
}

guint rcsharp_rc_resolvercontext_get_current_channel_offset (void);

guint
rcsharp_rc_resolvercontext_get_current_channel_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolverContext, current_channel);
}

guint rcsharp_rc_resolvercontext_get_min_priority_offset (void);

guint
rcsharp_rc_resolvercontext_get_min_priority_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolverContext, min_priority);
}

guint rcsharp_rc_resolverqueue_get_items_offset (void);

guint
rcsharp_rc_resolverqueue_get_items_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCResolverQueue, items);
}

guint rcsharp_rc_verification_get_info_offset (void);

guint
rcsharp_rc_verification_get_info_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCVerification, info);
}

guint rcsharp_rc_verification_get_type_offset (void);

guint
rcsharp_rc_verification_get_type_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCVerification, type);
}

guint rcsharp_rc_verification_get_status_offset (void);

guint
rcsharp_rc_verification_get_status_offset (void)
{
	return (guint)G_STRUCT_OFFSET (RCVerification, status);
}
