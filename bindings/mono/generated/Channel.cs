// This file was generated by the Gtk# code generator.
// Any changes made will be lost if regenerated.

namespace RC {

	using System;
	using System.Collections;
	using System.Runtime.InteropServices;

#region Autogenerated code
	public class Channel : GLib.Opaque {

		[DllImport("libredcarpet")]
		static extern IntPtr rc_channel_get_packages(IntPtr raw);

		public GLib.SList Packages { 
			get {
				IntPtr raw_ret = rc_channel_get_packages(Handle);
				GLib.SList ret = new GLib.SList(raw_ret, typeof (RC.Package));
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_channel_get_type();

		public static GLib.GType GType { 
			get {
				IntPtr raw_ret = rc_channel_get_type();
				GLib.GType ret = new GLib.GType(raw_ret);
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern int rc_channel_priority_parse(string arg1);

		public static int PriorityParse(string arg1) {
			int raw_ret = rc_channel_priority_parse(arg1);
			int ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_channel_set_type(IntPtr raw, int type);

		public void SetType(RC.ChannelType type) {
			rc_channel_set_type(Handle, (int) type);
		}

		[DllImport("libredcarpet")]
		static extern void rc_channel_set_system(IntPtr raw);

		public void SetSystem() {
			rc_channel_set_system(Handle);
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_channel_get_legacy_id(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_channel_set_legacy_id(IntPtr raw, string legacy_id);

		public string LegacyId { 
			get {
				IntPtr raw_ret = rc_channel_get_legacy_id(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_channel_set_legacy_id(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_channel_get_icon_file(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_channel_set_icon_file(IntPtr raw, string icon_file);

		public string IconFile { 
			get {
				IntPtr raw_ret = rc_channel_get_icon_file(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_channel_set_icon_file(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_channel_get_alias(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_channel_set_alias(IntPtr raw, string path);

		public string Alias { 
			get {
				IntPtr raw_ret = rc_channel_get_alias(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_channel_set_alias(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_channel_set_hidden(IntPtr raw);

		public void SetHidden() {
			rc_channel_set_hidden(Handle);
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_channel_get_path(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_channel_set_path(IntPtr raw, string path);

		public string Path { 
			get {
				IntPtr raw_ret = rc_channel_get_path(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_channel_set_path(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern bool rc_channel_is_immutable(IntPtr raw);

		public bool IsImmutable { 
			get {
				bool raw_ret = rc_channel_is_immutable(Handle);
				bool ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_channel_get_description(IntPtr raw);

		public string Description { 
			get {
				IntPtr raw_ret = rc_channel_get_description(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_channel_set_id_prefix(IntPtr raw, string prefix);

		public string IdPrefix { 
			set {
				rc_channel_set_id_prefix(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_channel_get_id(IntPtr raw);

		public string Id { 
			get {
				IntPtr raw_ret = rc_channel_get_id(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_channel_get_file_path(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_channel_set_file_path(IntPtr raw, string file_path);

		public string FilePath { 
			get {
				IntPtr raw_ret = rc_channel_get_file_path(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_channel_set_file_path(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_channel_make_immutable(IntPtr raw);

		public void MakeImmutable() {
			rc_channel_make_immutable(Handle);
		}

		[DllImport("libredcarpet")]
		static extern bool rc_channel_equal(IntPtr raw, IntPtr b);

		public bool Equal(RC.Channel b) {
			bool raw_ret = rc_channel_equal(Handle, b.Handle);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern int rc_channel_get_channel_type(IntPtr raw);

		public RC.ChannelType ChannelType { 
			get {
				int raw_ret = rc_channel_get_channel_type(Handle);
				RC.ChannelType ret = (RC.ChannelType)raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_channel_ref(IntPtr raw);

		public RC.Channel Ref() {
			IntPtr raw_ret = rc_channel_ref(Handle);
			RC.Channel ret;
			if (raw_ret == IntPtr.Zero)
				ret = null;
			else
				ret = new RC.Channel(raw_ret);
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_channel_add_distro_target(IntPtr raw, string target);

		public void AddDistroTarget(string target) {
			rc_channel_add_distro_target(Handle, target);
		}

		[DllImport("libredcarpet")]
		static extern bool rc_channel_is_wildcard(IntPtr raw);

		public bool IsWildcard { 
			get {
				bool raw_ret = rc_channel_is_wildcard(Handle);
				bool ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_channel_get_name(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_channel_set_name(IntPtr raw, string path);

		public string Name { 
			get {
				IntPtr raw_ret = rc_channel_get_name(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_channel_set_name(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_channel_unref(IntPtr raw);

		public void Unref() {
			rc_channel_unref(Handle);
		}

		[DllImport("libredcarpet")]
		static extern bool rc_channel_is_system(IntPtr raw);

		public bool IsSystem { 
			get {
				bool raw_ret = rc_channel_is_system(Handle);
				bool ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern bool rc_channel_is_subscribed(IntPtr raw);

		public bool IsSubscribed { 
			get {
				bool raw_ret = rc_channel_is_subscribed(Handle);
				bool ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern void rc_channel_set_priorities(IntPtr raw, int subd_priority, int unsubd_priority);

		public void SetPriorities(int subd_priority, int unsubd_priority) {
			rc_channel_set_priorities(Handle, subd_priority, unsubd_priority);
		}

		[DllImport("libredcarpet")]
		static extern bool rc_channel_has_distro_target(IntPtr raw, string target);

		public bool HasDistroTarget(string target) {
			bool raw_ret = rc_channel_has_distro_target(Handle, target);
			bool ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern void rc_channel_set_subscription(IntPtr raw, bool subscribed);

		public bool Subscription { 
			set {
				rc_channel_set_subscription(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern int rc_channel_get_priority(IntPtr raw, bool is_subscribed);

		public int GetPriority(bool is_subscribed) {
			int raw_ret = rc_channel_get_priority(Handle, is_subscribed);
			int ret = raw_ret;
			return ret;
		}

		[DllImport("libredcarpet")]
		static extern bool rc_channel_is_hidden(IntPtr raw);

		public bool IsHidden { 
			get {
				bool raw_ret = rc_channel_is_hidden(Handle);
				bool ret = raw_ret;
				return ret;
			}
		}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_channel_get_pkginfo_file(IntPtr raw);

		[DllImport("libredcarpet")]
		static extern void rc_channel_set_pkginfo_file(IntPtr raw, string pkginfo_file);

		public string PkginfoFile { 
			get {
				IntPtr raw_ret = rc_channel_get_pkginfo_file(Handle);
				string ret = Marshal.PtrToStringAnsi(raw_ret);
				return ret;
			}
			set {
				rc_channel_set_pkginfo_file(Handle, value);
			}
		}

		[DllImport("libredcarpet")]
		static extern bool rc_channel_equal_id(IntPtr raw, string id);

		public bool EqualId(string id) {
			bool raw_ret = rc_channel_equal_id(Handle, id);
			bool ret = raw_ret;
			return ret;
		}

		public Channel(IntPtr raw) : base(raw) {}

		[DllImport("libredcarpet")]
		static extern IntPtr rc_channel_new(string id, string name, string alias, string description);

		public Channel (string id, string name, string alias, string description) 
		{
			Raw = rc_channel_new(id, name, alias, description);
		}

#endregion
#region Customized extensions
#line 1 "Channel.custom"
/* -*- Mode: csharp; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

//namespace {
    public static Channel SYSTEM = new Channel (new IntPtr (1));
    public static Channel ANY = new Channel (new IntPtr (2));
    public static Channel NON_SYSTEM = new Channel (new IntPtr (3));

    private bool pkginfoFileCompressed = false;

    public bool PkginfoFileCompressed
    {
        get { return this.pkginfoFileCompressed; }
        set { this.pkginfoFileCompressed = value; }
    }

    [DllImport("libredcarpet")]
    static extern IntPtr rc_channel_get_world(IntPtr raw);
                                                                                
    public RC.World World {
        get {
            IntPtr raw_ret = rc_channel_get_world(Handle);
            RC.World ret;
            if (raw_ret == IntPtr.Zero)
                ret = null;
            else
                ret = (RC.World) GLib.Object.GetObject(raw_ret);
            return ret;
        }
    }

    public void ToXml (System.Xml.XmlTextWriter writer)
    {
        writer.WriteStartElement ("channel");

        writer.WriteAttributeString ("id", this.Id);
        writer.WriteAttributeString ("name", this.Name);

        if (this.Alias != null)
            writer.WriteAttributeString ("alias", this.Alias);

        writer.WriteAttributeString ("subscribed", this.IsSubscribed ? "1" : "0");
        writer.WriteAttributeString ("priority_base", System.Xml.XmlConvert.ToString (this.GetPriority (true)));
        writer.WriteAttributeString ("priority_unsubd", System.Xml.XmlConvert.ToString (this.GetPriority (false)));

        writer.WriteEndElement ();
    }

    public static void ToXml (Channel[] channels, System.IO.Stream stream)
    {
        System.Xml.XmlTextWriter writer = new System.Xml.XmlTextWriter (stream, null);
        writer.WriteStartDocument ();
        writer.WriteStartElement ("channellist");

        foreach (Channel c in channels)
            c.ToXml (writer);

        writer.WriteEndElement ();
        writer.WriteEndDocument ();
        writer.Close ();
    }

    public static void ToXml (Channel[] channels, string filename)
    {
        System.IO.Stream s = System.IO.File.Open (filename, System.IO.FileMode.OpenOrCreate);
        Channel.ToXml (channels, s);
        s.Close ();
    }

    private static void FromXml (System.Xml.XmlTextReader reader, ChannelDelegate callback)
    {
        reader.MoveToContent ();
        reader.ReadStartElement ("channellist");

        while (reader.Read ()) {
            if (reader.LocalName != "channel")
                continue;

            if (!reader.HasAttributes) {
                Console.WriteLine ("channel doesn't have attributes, skipping");
                continue;
            }

            Channel c = new Channel (reader["bid"],
                                     reader["name"],
                                     reader["alias"],
                                     reader["description"]);
            c.LegacyId = reader["id"];

            string distros = reader["distro_target"];
            if (distros != null) {
                foreach (string d in distros.Split (new Char[] {':'}))
                    c.AddDistroTarget (d);
            }

            ChannelType type = ChannelType.Unknown;
            string type_str = reader["type"];
            if (type_str != null) {
                switch (type_str) {
                case "helix":
                    type = ChannelType.Helix;
                    break;
                case "debian":
                    type = ChannelType.Debian;
                    break;
                case "aptrpm":
                    type = ChannelType.Aptrpm;
                    break;
                case "yum":
                    type = ChannelType.Yum;
                    break;
                default:
                    break;
                }
            }

            c.SetType (type);

            int subd_priority = 0;
            int unsubd_priority = 0;
            string priority = reader["priority"];
            if (priority != null)
                subd_priority = Channel.PriorityParse (priority);

            priority = reader["priority_when_unsubscribed"];
            if (priority != null)
                unsubd_priority = Channel.PriorityParse (priority);

            c.SetPriorities (subd_priority, unsubd_priority);

            c.Path = reader["path"];
            c.FilePath = reader["file_path"];
            c.IconFile = reader["icon"];
            c.PkginfoFile = reader["pkginfo_file"];

            string compressed = reader["pkginfo_compressed"];
            if (compressed != null && compressed == "1")
                c.PkginfoFileCompressed = true;

            if (!callback (c))
                break;
        }
    }

    public static void FromXml (System.IO.Stream stream, ChannelDelegate callback)
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

        public bool ForeachChannelCb (Channel channel) {
            this.list.Add (channel);
            return true;
        }

        public Channel[] Channels {
            get { return (Channel[]) this.list.ToArray (typeof (Channel)); }
        }
    }

    public static Channel[] FromXml (System.IO.Stream stream)
    {
        FromXmlHelper helper = new FromXmlHelper ();
        FromXml (stream, new ChannelDelegate (helper.ForeachChannelCb));
        return helper.Channels;
    }

#endregion
	}
}