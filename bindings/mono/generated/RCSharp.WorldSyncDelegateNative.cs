// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RCSharp {

	using System;

#region Autogenerated code
	[GLib.CDeclCallback]
	internal delegate bool WorldSyncDelegateNative(IntPtr world, IntPtr channel);

	internal class WorldSyncDelegateWrapper {

		public bool NativeCallback (IntPtr world, IntPtr channel)
		{
			RC.World _arg0 = GLib.Object.GetObject(world) as RC.World;
			RC.Channel _arg1 = channel == IntPtr.Zero ? null : (RC.Channel) GLib.Opaque.GetOpaque (channel, typeof (RC.Channel), false);
			return (bool) managed ( _arg0,  _arg1);
		}

		internal WorldSyncDelegateNative NativeDelegate;
		RC.WorldSyncDelegate managed;

		public WorldSyncDelegateWrapper (RC.WorldSyncDelegate managed)
		{
			this.managed = managed;
			if (managed != null)
				NativeDelegate = new WorldSyncDelegateNative (NativeCallback);
		}

		public static RC.WorldSyncDelegate GetManagedDelegate (WorldSyncDelegateNative native)
		{
			if (native == null)
				return null;
			WorldSyncDelegateWrapper wrapper = (WorldSyncDelegateWrapper) native.Target;
			if (wrapper == null)
				return null;
			return wrapper.managed;
		}
	}
#endregion
}
