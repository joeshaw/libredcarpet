// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	[StructLayout(LayoutKind.Sequential)]
	public struct ResolverQueue {

		private IntPtr _context;

		public RC.ResolverContext context {
			get { 
				RC.ResolverContext ret = new RC.ResolverContext(_context);
				if (ret == null) ret = new RC.ResolverContext(_context);
				return ret;
			}
			set { _context = value.Handle; }
		}
		private IntPtr _items;

		public static RC.ResolverQueue Zero = new RC.ResolverQueue ();

		public static RC.ResolverQueue New(IntPtr raw) {
			if (raw == IntPtr.Zero) {
				return RC.ResolverQueue.Zero;
			}
			RC.ResolverQueue self = new RC.ResolverQueue();
			self = (RC.ResolverQueue) Marshal.PtrToStructure (raw, self.GetType ());
			return self;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_resolver_queue_new();

		public static ResolverQueue New()
		{
			return ResolverQueue.New (rc_resolver_queue_new());
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_resolver_queue_new_with_context(IntPtr arg1);

		public static ResolverQueue NewWithContext(RC.ResolverContext arg1)
		{
			return ResolverQueue.New (rc_resolver_queue_new_with_context(arg1.Handle));
		}

		[DllImport("libredcarpet")]
		static extern void rc_resolver_queue_add_package_to_remove(ref RC.ResolverQueue raw, IntPtr package, bool remove_only_mode);

		public void AddPackageToRemove(RC.Package package, bool remove_only_mode) {
			rc_resolver_queue_add_package_to_remove(ref this, package.Handle, remove_only_mode);
		}

		[DllImport("libredcarpet")]
		static extern bool rc_resolver_queue_is_invalid(ref RC.ResolverQueue raw);

		public bool IsInvalid { 
			get {
				bool raw_ret = rc_resolver_queue_is_invalid(ref this);
				bool ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_resolver_queue_process(ref RC.ResolverQueue raw);

		public void Process() {
			rc_resolver_queue_process(ref this);
		}

		[DllImport("libredcarpet")]
		static extern void rc_resolver_queue_add_package_to_install(ref RC.ResolverQueue raw, IntPtr package);

		public void AddPackageToInstall(RC.Package package) {
			rc_resolver_queue_add_package_to_install(ref this, package.Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_resolver_queue_split_first_branch(ref RC.ResolverQueue raw, IntPtr new_queues, IntPtr deferred_queues);

		public void SplitFirstBranch(GLib.SList new_queues, GLib.SList deferred_queues) {
			rc_resolver_queue_split_first_branch(ref this, new_queues.Handle, deferred_queues.Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_resolver_queue_add_item(ref RC.ResolverQueue raw, ref RC.QueueItem item);

		public void AddItem(RC.QueueItem item) {
			rc_resolver_queue_add_item(ref this, ref item);
		}

		[DllImport("libredcarpet")]
		static extern bool rc_resolver_queue_contains_only_branches(ref RC.ResolverQueue raw);

		public bool ContainsOnlyBranches() {
			bool raw_ret = rc_resolver_queue_contains_only_branches(ref this);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_resolver_queue_add_package_to_verify(ref RC.ResolverQueue raw, IntPtr package);

		public void AddPackageToVerify(RC.Package package) {
			rc_resolver_queue_add_package_to_verify(ref this, package.Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_resolver_queue_free(ref RC.ResolverQueue raw);

		public void Free() {
			rc_resolver_queue_free(ref this);
		}

		[DllImport("libredcarpet")]
		static extern bool rc_resolver_queue_process_once(ref RC.ResolverQueue raw);

		public bool ProcessOnce() {
			bool raw_ret = rc_resolver_queue_process_once(ref this);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_resolver_queue_add_extra_dependency(ref RC.ResolverQueue raw, IntPtr dep);

		public void AddExtraDependency(RC.PackageDep dep) {
			rc_resolver_queue_add_extra_dependency(ref this, dep.Handle);
		}

		[DllImport("libredcarpet")]
		static extern void rc_resolver_queue_add_extra_conflict(ref RC.ResolverQueue raw, IntPtr dep);

		public void AddExtraConflict(RC.PackageDep dep) {
			rc_resolver_queue_add_extra_conflict(ref this, dep.Handle);
		}

		[DllImport("libredcarpet")]
		static extern bool rc_resolver_queue_is_empty(ref RC.ResolverQueue raw);

		public bool IsEmpty { 
			get {
				bool raw_ret = rc_resolver_queue_is_empty(ref this);
				bool ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_resolver_queue_spew(ref RC.ResolverQueue raw);

		public void Spew() {
			rc_resolver_queue_spew(ref this);
		}

#endregion
	}
}
