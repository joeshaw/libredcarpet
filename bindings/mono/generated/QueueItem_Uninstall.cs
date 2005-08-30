// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	[StructLayout(LayoutKind.Sequential)]
	public struct QueueItem_Uninstall {

		private IntPtr _parent;
		public RC.QueueItem Parent {
			get {
				return _parent == IntPtr.Zero ? null : (RC.QueueItem) GLib.Opaque.GetOpaque (_parent, typeof (RC.QueueItem), false);
			}
			set {
				_parent = value == null ? IntPtr.Zero : value.Handle;
			}
		}
		private IntPtr _package;
		public RC.Package Package {
			get {
				return _package == IntPtr.Zero ? null : (RC.Package) GLib.Opaque.GetOpaque (_package, typeof (RC.Package), false);
			}
			set {
				_package = value == null ? IntPtr.Zero : value.Handle;
			}
		}
		public string Reason;
		private IntPtr _dep_leading_to_uninstall;
		public RC.PackageDep DepLeadingToUninstall {
			get {
				return _dep_leading_to_uninstall == IntPtr.Zero ? null : (RC.PackageDep) GLib.Opaque.GetOpaque (_dep_leading_to_uninstall, typeof (RC.PackageDep), false);
			}
			set {
				_dep_leading_to_uninstall = value == null ? IntPtr.Zero : value.Handle;
			}
		}
		private IntPtr _upgraded_to;
		public RC.Package UpgradedTo {
			get {
				return _upgraded_to == IntPtr.Zero ? null : (RC.Package) GLib.Opaque.GetOpaque (_upgraded_to, typeof (RC.Package), false);
			}
			set {
				_upgraded_to = value == null ? IntPtr.Zero : value.Handle;
			}
		}
		private uint _bitfield0;

		[DllImport ("libredcarpetsharpglue-0")]
		extern static bool rcsharp_rc_queueitem_uninstall_get_explicitly_requested (ref RC.QueueItem_Uninstall raw);
		[DllImport ("libredcarpetsharpglue-0")]
		extern static void rcsharp_rc_queueitem_uninstall_set_explicitly_requested (ref RC.QueueItem_Uninstall raw, bool value);
		public bool ExplicitlyRequested {
			get {
				return rcsharp_rc_queueitem_uninstall_get_explicitly_requested (ref this);
			}
			set {
				rcsharp_rc_queueitem_uninstall_set_explicitly_requested (ref this, value);
			}
		}

		[DllImport ("libredcarpetsharpglue-0")]
		extern static bool rcsharp_rc_queueitem_uninstall_get_remove_only (ref RC.QueueItem_Uninstall raw);
		[DllImport ("libredcarpetsharpglue-0")]
		extern static void rcsharp_rc_queueitem_uninstall_set_remove_only (ref RC.QueueItem_Uninstall raw, bool value);
		public bool RemoveOnly {
			get {
				return rcsharp_rc_queueitem_uninstall_get_remove_only (ref this);
			}
			set {
				rcsharp_rc_queueitem_uninstall_set_remove_only (ref this, value);
			}
		}

		[DllImport ("libredcarpetsharpglue-0")]
		extern static bool rcsharp_rc_queueitem_uninstall_get_due_to_conflict (ref RC.QueueItem_Uninstall raw);
		[DllImport ("libredcarpetsharpglue-0")]
		extern static void rcsharp_rc_queueitem_uninstall_set_due_to_conflict (ref RC.QueueItem_Uninstall raw, bool value);
		public bool DueToConflict {
			get {
				return rcsharp_rc_queueitem_uninstall_get_due_to_conflict (ref this);
			}
			set {
				rcsharp_rc_queueitem_uninstall_set_due_to_conflict (ref this, value);
			}
		}

		[DllImport ("libredcarpetsharpglue-0")]
		extern static bool rcsharp_rc_queueitem_uninstall_get_due_to_obsolete (ref RC.QueueItem_Uninstall raw);
		[DllImport ("libredcarpetsharpglue-0")]
		extern static void rcsharp_rc_queueitem_uninstall_set_due_to_obsolete (ref RC.QueueItem_Uninstall raw, bool value);
		public bool DueToObsolete {
			get {
				return rcsharp_rc_queueitem_uninstall_get_due_to_obsolete (ref this);
			}
			set {
				rcsharp_rc_queueitem_uninstall_set_due_to_obsolete (ref this, value);
			}
		}

		[DllImport ("libredcarpetsharpglue-0")]
		extern static bool rcsharp_rc_queueitem_uninstall_get_unlink (ref RC.QueueItem_Uninstall raw);
		[DllImport ("libredcarpetsharpglue-0")]
		extern static void rcsharp_rc_queueitem_uninstall_set_unlink (ref RC.QueueItem_Uninstall raw, bool value);
		public bool Unlink {
			get {
				return rcsharp_rc_queueitem_uninstall_get_unlink (ref this);
			}
			set {
				rcsharp_rc_queueitem_uninstall_set_unlink (ref this, value);
			}
		}


		public static RC.QueueItem_Uninstall Zero = new RC.QueueItem_Uninstall ();

		public static RC.QueueItem_Uninstall New(IntPtr raw) {
			if (raw == IntPtr.Zero) {
				return RC.QueueItem_Uninstall.Zero;
			}
			RC.QueueItem_Uninstall self = new RC.QueueItem_Uninstall();
			self = (RC.QueueItem_Uninstall) Marshal.PtrToStructure (raw, self.GetType ());
			return self;
		}

		private static GLib.GType GType {
			get { return GLib.GType.Pointer; }
		}
#endregion
	}
}
