// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace GtkSharp.LibredcarpetSharp {

	public class ObjectManager {

		static bool initialized = false;
		// Call this method from the appropriate module init function.
		public static void Initialize ()
		{
			if (initialized)
				return;

			initialized = true;
			GLib.ObjectManager.RegisterType("RCWorldUndump", "RC.WorldUndump,libredcarpet-sharp");
			GLib.ObjectManager.RegisterType("RCWorldSystem", "RC.WorldSystem,libredcarpet-sharp");
			GLib.ObjectManager.RegisterType("RCWorld", "RC.World,libredcarpet-sharp");
			GLib.ObjectManager.RegisterType("RCWorldLocalDir", "RC.WorldLocalDir,libredcarpet-sharp");
			GLib.ObjectManager.RegisterType("RCWorldMulti", "RC.WorldMulti,libredcarpet-sharp");
			GLib.ObjectManager.RegisterType("RCWorldStore", "RC.WorldStore,libredcarpet-sharp");
			GLib.ObjectManager.RegisterType("RCWorldService", "RC.WorldService,libredcarpet-sharp");
			GLib.ObjectManager.RegisterType("RCPending", "RC.Pending,libredcarpet-sharp");
			GLib.ObjectManager.RegisterType("RCWorldSynthetic", "RC.WorldSynthetic,libredcarpet-sharp");
			GLib.ObjectManager.RegisterType("RCPackman", "RC.Packman,libredcarpet-sharp");
		}
	}
}