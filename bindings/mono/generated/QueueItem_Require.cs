// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	[StructLayout(LayoutKind.Sequential)]
	public struct QueueItem_Require {

		public RC.QueueItem Parent;
		private IntPtr _dep;

		public RC.PackageDep dep {
			get { 
				RC.PackageDep ret = new RC.PackageDep(_dep);
				if (ret == null) ret = new RC.PackageDep(_dep);
				return ret;
			}
			set { _dep = value.Handle; }
		}
		private IntPtr _requiring_package;

		public RC.Package requiring_package {
			get { 
				RC.Package ret = new RC.Package(_requiring_package);
				if (ret == null) ret = new RC.Package(_requiring_package);
				return ret;
			}
			set { _requiring_package = value.Handle; }
		}
		private IntPtr _upgraded_package;

		public RC.Package upgraded_package {
			get { 
				RC.Package ret = new RC.Package(_upgraded_package);
				if (ret == null) ret = new RC.Package(_upgraded_package);
				return ret;
			}
			set { _upgraded_package = value.Handle; }
		}
		private IntPtr _lost_package;

		public RC.Package lost_package {
			get { 
				RC.Package ret = new RC.Package(_lost_package);
				if (ret == null) ret = new RC.Package(_lost_package);
				return ret;
			}
			set { _lost_package = value.Handle; }
		}
		private uint _bitfield0;
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
