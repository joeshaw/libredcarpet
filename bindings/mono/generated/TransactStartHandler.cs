// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;

	public delegate void TransactStartHandler(object o, TransactStartArgs args);

	public class TransactStartArgs : GLib.SignalArgs {
		public int TotalSteps{
			get {
				return (int) Args[0];
			}
		}

	}
}
