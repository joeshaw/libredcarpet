// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RCSharp {

	using System;

#region Autogenerated code
	internal delegate bool WorldCanTransactPackageDelegateNative(IntPtr world, IntPtr package);

	internal class WorldCanTransactPackageDelegateWrapper : GLib.DelegateWrapper {

		public bool NativeCallback (IntPtr world, IntPtr package)
		{
			RC.World _arg0 = (RC.World) GLib.Object.GetObject(world);
			RC.Package _arg1 = new RC.Package(package);
			return (bool) _managed ( _arg0,  _arg1);
		}

		internal WorldCanTransactPackageDelegateNative NativeDelegate;
		protected RC.WorldCanTransactPackageDelegate _managed;

		public WorldCanTransactPackageDelegateWrapper (RC.WorldCanTransactPackageDelegate managed, object o) : base (o)
		{
			NativeDelegate = new WorldCanTransactPackageDelegateNative (NativeCallback);
			_managed = managed;
		}
	}
#endregion
}
