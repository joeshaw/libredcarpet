// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RCSharp {

	using System;

#region Autogenerated code
	internal delegate void ResolverContextDelegateNative(IntPtr ctx, IntPtr data);

	internal class ResolverContextDelegateWrapper : GLib.DelegateWrapper {

		public void NativeCallback (IntPtr ctx, IntPtr data)
		{
			RC.ResolverContext _arg0 = new RC.ResolverContext(ctx);
			_managed ( _arg0);
		}

		internal ResolverContextDelegateNative NativeDelegate;
		protected RC.ResolverContextDelegate _managed;

		public ResolverContextDelegateWrapper (RC.ResolverContextDelegate managed, object o) : base (o)
		{
			NativeDelegate = new ResolverContextDelegateNative (NativeCallback);
			_managed = managed;
		}
	}
#endregion
}
