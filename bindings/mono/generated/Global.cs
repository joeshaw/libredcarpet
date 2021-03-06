// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public class Global {

		[DllImport("libredcarpet")]
		static extern bool rc_url_is_absolute(IntPtr url);

		public static bool UrlIsAbsolute(string url) {
			IntPtr url_as_native = GLib.Marshaller.StringToPtrGStrdup (url);
			bool raw_ret = rc_url_is_absolute(url_as_native);
			bool ret = raw_ret;
			GLib.Marshaller.Free (url_as_native);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_arch_get_compat_list(int arch);

		public static GLib.SList ArchGetCompatList(RC.Arch arch) {
			IntPtr raw_ret = rc_arch_get_compat_list((int) arch);
			GLib.SList ret = new GLib.SList(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern int rc_arch_get_system_arch();

		public static RC.Arch ArchGetSystemArch() {
			int raw_ret = rc_arch_get_system_arch();
			RC.Arch ret = (RC.Arch) raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern int rc_arch_from_string(IntPtr arch_name);

		public static RC.Arch ArchFromString(string arch_name) {
			IntPtr arch_name_as_native = GLib.Marshaller.StringToPtrGStrdup (arch_name);
			int raw_ret = rc_arch_from_string(arch_name_as_native);
			RC.Arch ret = (RC.Arch) raw_ret;
			GLib.Marshaller.Free (arch_name_as_native);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern int rc_arch_get_compat_score(IntPtr compat_arch_list, int arch);

		public static int ArchGetCompatScore(GLib.SList compat_arch_list, RC.Arch arch) {
			int raw_ret = rc_arch_get_compat_score(compat_arch_list.Handle, (int) arch);
			int ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern int rc_string_to_package_importance(IntPtr importance);

		public static RC.PackageImportance PackageImportanceFromString(string importance) {
			IntPtr importance_as_native = GLib.Marshaller.StringToPtrGStrdup (importance);
			int raw_ret = rc_string_to_package_importance(importance_as_native);
			RC.PackageImportance ret = (RC.PackageImportance) raw_ret;
			GLib.Marshaller.Free (importance_as_native);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_maybe_merge_paths(IntPtr parent_path, IntPtr child_path);

		public static string MaybeMergePaths(string parent_path, string child_path) {
			IntPtr parent_path_as_native = GLib.Marshaller.StringToPtrGStrdup (parent_path);
			IntPtr child_path_as_native = GLib.Marshaller.StringToPtrGStrdup (child_path);
			IntPtr raw_ret = rc_maybe_merge_paths(parent_path_as_native, child_path_as_native);
			string ret = GLib.Marshaller.PtrToStringGFree(raw_ret);
			GLib.Marshaller.Free (parent_path_as_native);
			GLib.Marshaller.Free (child_path_as_native);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_importance_to_string(int importance);

		public static string ImportanceToString(RC.PackageImportance importance) {
			IntPtr raw_ret = rc_package_importance_to_string((int) importance);
			string ret = GLib.Marshaller.Utf8PtrToString (raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern int rc_string_to_package_section(IntPtr section);

		public static RC.PackageSection StringToPackageSection(string section) {
			IntPtr section_as_native = GLib.Marshaller.StringToPtrGStrdup (section);
			int raw_ret = rc_string_to_package_section(section_as_native);
			RC.PackageSection ret = (RC.PackageSection) raw_ret;
			GLib.Marshaller.Free (section_as_native);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_arch_to_string(int arch);

		public static string ArchToString(RC.Arch arch) {
			IntPtr raw_ret = rc_arch_to_string((int) arch);
			string ret = GLib.Marshaller.Utf8PtrToString (raw_ret);
			return ret;
		}

#endregion
#region Customized extensions
#line 1 "Global.custom"
public static void Init ()
{
    GtkSharp.LibredcarpetSharp.ObjectManager.Initialize ();
}

#endregion
	}
}
