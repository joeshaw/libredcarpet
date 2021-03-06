// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RCSharp {

	using System;

#region Autogenerated code
	[GLib.CDeclCallback]
	internal delegate bool PackageAndSpecDelegateNative(IntPtr pkg, IntPtr spec, IntPtr data);

	internal class PackageAndSpecDelegateWrapper {

		public bool NativeCallback (IntPtr pkg, IntPtr spec, IntPtr data)
		{
			RC.Package _arg0 = pkg == IntPtr.Zero ? null : (RC.Package) GLib.Opaque.GetOpaque (pkg, typeof (RC.Package), false);
			RC.PackageSpec _arg1 = new RC.PackageSpec(spec);
			return (bool) managed ( _arg0,  _arg1);
		}

		internal PackageAndSpecDelegateNative NativeDelegate;
		RC.PackageAndSpecDelegate managed;

		public PackageAndSpecDelegateWrapper (RC.PackageAndSpecDelegate managed)
		{
			this.managed = managed;
			if (managed != null)
				NativeDelegate = new PackageAndSpecDelegateNative (NativeCallback);
		}

		public static RC.PackageAndSpecDelegate GetManagedDelegate (PackageAndSpecDelegateNative native)
		{
			if (native == null)
				return null;
			PackageAndSpecDelegateWrapper wrapper = (PackageAndSpecDelegateWrapper) native.Target;
			if (wrapper == null)
				return null;
			return wrapper.managed;
		}
	}
#endregion
}
