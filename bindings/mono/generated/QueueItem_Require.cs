// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	[StructLayout(LayoutKind.Sequential)]
	public struct QueueItem_Require {

		private IntPtr _parent;
		public RC.QueueItem Parent {
			get {
				return _parent == IntPtr.Zero ? null : (RC.QueueItem) GLib.Opaque.GetOpaque (_parent, typeof (RC.QueueItem), false);
			}
			set {
				_parent = value == null ? IntPtr.Zero : value.Handle;
			}
		}
		private IntPtr _dep;
		public RC.PackageDep Dep {
			get {
				return _dep == IntPtr.Zero ? null : (RC.PackageDep) GLib.Opaque.GetOpaque (_dep, typeof (RC.PackageDep), false);
			}
			set {
				_dep = value == null ? IntPtr.Zero : value.Handle;
			}
		}
		private IntPtr _requiring_package;
		public RC.Package RequiringPackage {
			get {
				return _requiring_package == IntPtr.Zero ? null : (RC.Package) GLib.Opaque.GetOpaque (_requiring_package, typeof (RC.Package), false);
			}
			set {
				_requiring_package = value == null ? IntPtr.Zero : value.Handle;
			}
		}
		private IntPtr _upgraded_package;
		public RC.Package UpgradedPackage {
			get {
				return _upgraded_package == IntPtr.Zero ? null : (RC.Package) GLib.Opaque.GetOpaque (_upgraded_package, typeof (RC.Package), false);
			}
			set {
				_upgraded_package = value == null ? IntPtr.Zero : value.Handle;
			}
		}
		private IntPtr _lost_package;
		public RC.Package LostPackage {
			get {
				return _lost_package == IntPtr.Zero ? null : (RC.Package) GLib.Opaque.GetOpaque (_lost_package, typeof (RC.Package), false);
			}
			set {
				_lost_package = value == null ? IntPtr.Zero : value.Handle;
			}
		}
		private uint _bitfield0;

		[DllImport ("libredcarpetsharpglue-0")]
		extern static bool rcsharp_rc_queueitem_require_get_remove_only (ref RC.QueueItem_Require raw);
		[DllImport ("libredcarpetsharpglue-0")]
		extern static void rcsharp_rc_queueitem_require_set_remove_only (ref RC.QueueItem_Require raw, bool value);
		public bool RemoveOnly {
			get {
				return rcsharp_rc_queueitem_require_get_remove_only (ref this);
			}
			set {
				rcsharp_rc_queueitem_require_set_remove_only (ref this, value);
			}
		}

		public bool IsChild;

		public static RC.QueueItem_Require Zero = new RC.QueueItem_Require ();

		public static RC.QueueItem_Require New(IntPtr raw) {
			if (raw == IntPtr.Zero) {
				return RC.QueueItem_Require.Zero;
			}
			RC.QueueItem_Require self = new RC.QueueItem_Require();
			self = (RC.QueueItem_Require) Marshal.PtrToStructure (raw, self.GetType ());
			return self;
		}

		private static GLib.GType GType {
			get { return GLib.GType.Pointer; }
		}
#endregion
	}
}
