// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RCSharp {

	using System;
	using System.Runtime.InteropServices;

	internal delegate void voidObjectintDelegate(IntPtr arg0, int arg1, int key);

	internal class voidObjectintSignal : GLib.SignalCallback {

		private static voidObjectintDelegate _Delegate;

		private static void voidObjectintCallback(IntPtr arg0, int arg1, int key)
		{
			if (!_Instances.Contains(key))
				throw new Exception("Unexpected signal key " + key);

			voidObjectintSignal inst = (voidObjectintSignal) _Instances[key];
			GLib.SignalArgs args = (GLib.SignalArgs) Activator.CreateInstance (inst._argstype);
			args.Args = new object[1];
			args.Args[0] = arg1;
			object[] argv = new object[2];
			argv[0] = inst._obj;
			argv[1] = args;
			inst._handler.DynamicInvoke(argv);
		}

		public voidObjectintSignal(GLib.Object obj, string name, Delegate eh, Type argstype, int connect_flags) : base(obj, eh, argstype)
		{
			if (_Delegate == null) {
				_Delegate = new voidObjectintDelegate(voidObjectintCallback);
			}
			Connect (name, _Delegate, connect_flags);
		}

		protected override void Dispose (bool disposing)
		{
			_Instances.Remove(_key);
			if(_Instances.Count == 0)
				_Delegate = null;

			Disconnect ();
			base.Dispose (disposing);
		}
	}
}