// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public class PackageMatch : GLib.Opaque {

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_match_get_channel_id(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_match_set_channel_id(IntPtr raw, string cid);

		public string ChannelId { 
			get {
				IntPtr raw_ret = rc_package_match_get_channel_id(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_package_match_set_channel_id(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern bool rc_package_match_equal(IntPtr raw, IntPtr match2);

		public bool Equal(RC.PackageMatch match2) {
			bool raw_ret = rc_package_match_equal(Handle, match2.Handle);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_match_get_glob(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_match_set_glob(IntPtr raw, string glob_str);

		public string Glob { 
			get {
				IntPtr raw_ret = rc_package_match_get_glob(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_package_match_set_glob(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern int rc_package_match_get_importance(IntPtr raw, out bool match_gteq);

		public RC.PackageImportance GetImportance(out bool match_gteq) {
			int raw_ret = rc_package_match_get_importance(Handle, out match_gteq);
			RC.PackageImportance ret = (RC.PackageImportance)raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_package_match_set_importance(IntPtr raw, int importance, bool match_gteq);

		public void SetImportance(RC.PackageImportance importance, bool match_gteq) {
			rc_package_match_set_importance(Handle, (int) importance, match_gteq);
		}

		[DllImport("libredcarpet")]
		static extern void rc_package_match_set_channel(IntPtr raw, IntPtr channel);

		public RC.Channel Channel { 
			set {
				rc_package_match_set_channel(Handle, value.Handle);
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_package_match_free(IntPtr raw);

		public void Free() {
			rc_package_match_free(Handle);
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_match_get_dep(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_package_match_set_dep(IntPtr raw, IntPtr dep);

		public RC.PackageDep Dep { 
			get {
				IntPtr raw_ret = rc_package_match_get_dep(Handle);
				RC.PackageDep ret;
				if (raw_ret == IntPtr.Zero)
					ret = null;
				else
					ret = new RC.PackageDep(raw_ret);
				return ret;
			}
			set {
				rc_package_match_set_dep(Handle, value.Handle);
			}
		}

		public PackageMatch(IntPtr raw) : base(raw) {}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_package_match_new();

		public PackageMatch () 
		{
			Raw = rc_package_match_new();
		}

#endregion
#region Customized extensions
#line 1 "PackageMatch.custom"
/* -*- Mode: csharp; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

//namespace {
    public void ToXml (System.Xml.XmlTextWriter writer)
    {
        writer.WriteStartElement ("match");

        if (this.ChannelId != null)
            writer.WriteElementString ("channel", this.ChannelId);

        if (this.Dep != null)
            this.Dep.ToXml (writer);

        if (this.Glob != null)
            writer.WriteElementString ("glob", this.Glob);

        bool gteq;
        PackageImportance imp = this.GetImportance (out gteq);
        if (imp != PackageImportance.Invalid) {
            writer.WriteStartElement ("importance", Global.ImportanceToString (imp));
            writer.WriteAttributeString ("gteq", gteq ? "1" : "0");
        }

        writer.WriteEndElement ();
    }

    private static PackageMatch ParseMatch (System.Xml.XmlTextReader reader)
    {
        // Make sure we're in right position
        reader.ReadStartElement ("match");

        PackageMatch match = new PackageMatch ();

        if (reader.LocalName == "channel")
            match.ChannelId = reader.ReadElementString ();

        if (reader.LocalName == "dep") {
            PackageDep dep = new PackageDep (reader);
            match.Dep = dep;
            dep.Unref ();

            // Move reader to start of next element
            while (reader.Read ())
                if (reader.NodeType == System.Xml.XmlNodeType.Element)
                    break;
        }

        if (reader.LocalName == "glob")
            match.Glob = reader.ReadElementString ();

        if (reader.LocalName == "importance") {
            int gteq = 0;
            string gteq_str = reader["gteq"];
            if (gteq_str != null)
                gteq = System.Xml.XmlConvert.ToInt32 (gteq_str);

            PackageImportance imp = RC.Global.PackageImportanceFromString (reader.ReadElementString ());
            match.SetImportance (imp, Convert.ToBoolean (gteq));
        }

        return match;
    }

    private static void FromXml (System.Xml.XmlTextReader reader, PackageMatchDelegate callback)
    {
        reader.MoveToContent ();

        while (reader.Read ()) {
            if (reader.NodeType == System.Xml.XmlNodeType.Element && reader.LocalName == "match") {
                PackageMatch match = ParseMatch (reader);

                if (!callback (match))
                    break;
            }
        }

        reader.Close ();
    }

    public static void FromXml (System.IO.Stream stream, PackageMatchDelegate callback)
    {
        System.Xml.XmlTextReader reader = new System.Xml.XmlTextReader (stream);
        FromXml (reader, callback);
        reader.Close ();
        stream.Close ();
    }

    private class FromXmlHelper {
        private System.Collections.ArrayList list;

        public FromXmlHelper () {
            this.list = new System.Collections.ArrayList ();
        }

        public bool ForeachMatchCb (PackageMatch match) {
            this.list.Add (match);
            return true;
        }

        public PackageMatch[] Matches {
            get { return (PackageMatch[]) this.list.ToArray (typeof (PackageMatch)); }
        }
    }

    public static PackageMatch[] FromXml (System.IO.Stream stream)
    {
        FromXmlHelper helper = new FromXmlHelper ();
        FromXml (stream, new PackageMatchDelegate (helper.ForeachMatchCb));
        return helper.Matches;
    }

#endregion
	}
}
