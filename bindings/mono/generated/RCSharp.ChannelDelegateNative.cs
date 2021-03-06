// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RCSharp {

	using System;

#region Autogenerated code
	[GLib.CDeclCallback]
	internal delegate bool ChannelDelegateNative(IntPtr channel, IntPtr data);

	internal class ChannelDelegateWrapper {

		public bool NativeCallback (IntPtr channel, IntPtr data)
		{
			RC.Channel _arg0 = channel == IntPtr.Zero ? null : (RC.Channel) GLib.Opaque.GetOpaque (channel, typeof (RC.Channel), false);
			return (bool) managed ( _arg0);
		}

		internal ChannelDelegateNative NativeDelegate;
		RC.ChannelDelegate managed;

		public ChannelDelegateWrapper (RC.ChannelDelegate managed)
		{
			this.managed = managed;
			if (managed != null)
				NativeDelegate = new ChannelDelegateNative (NativeCallback);
		}

		public static RC.ChannelDelegate GetManagedDelegate (ChannelDelegateNative native)
		{
			if (native == null)
				return null;
			ChannelDelegateWrapper wrapper = (ChannelDelegateWrapper) native.Target;
			if (wrapper == null)
				return null;
			return wrapper.managed;
		}
	}
#endregion
}
