#pragma once

struct proxy_settings_s
{
    int proxy_type = 0;
    ts::str_c proxy_addr;
    proxy_settings_s() {}
    proxy_settings_s(const proxy_settings_s &p) :proxy_type(p.proxy_type)
    {
        proxy_addr.setcopy(p.proxy_addr); // make copy to avoid race condition
    }
    void operator=(const proxy_settings_s &p)
    {
        proxy_type = p.proxy_type;
        proxy_addr.setcopy(p.proxy_addr); // make copy to avoid race condition
    }
    void operator=(proxy_settings_s &&p)
    {
        proxy_type = p.proxy_type;
        proxy_addr = std::move(p.proxy_addr);
        p.proxy_addr.clear();
    }
    bool operator!=(const proxy_settings_s &p) const
    {
        return proxy_type != p.proxy_type || !proxy_addr.equals(p.proxy_addr);
    }

};

struct configurable_s
{
    proxy_settings_s proxy;
    int server_port = 0;
    bool ipv6_enable = true;
    bool udp_enable = true;
    bool initialized = false;

    bool operator != (const configurable_s &o) const
    {
        return initialized != o.initialized || proxy != o.proxy || server_port != o.server_port || ipv6_enable != o.ipv6_enable || udp_enable != o.udp_enable;
    }
    configurable_s() {}
    configurable_s(const configurable_s &c)
    {
        *this = c;
    }
    configurable_s &operator=(const configurable_s &c)
    {
        if (!c.initialized) return *this;
        proxy = c.proxy;
        server_port = c.server_port;
        ipv6_enable = c.ipv6_enable;
        udp_enable = c.udp_enable;
        initialized = true;
        return *this;
    }
};

struct active_protocol_data_s
{
    ts::str_c tag;
    ts::str_c name; // utf8
    ts::str_c user_name; // utf8
    ts::str_c user_statusmsg; // utf8
    ts::blob_c config;
    ts::blob_c avatar;
    int sort_factor = 0;
    int options = O_AUTOCONNECT;

    configurable_s configurable;


    enum options_e
    {
        O_AUTOCONNECT   = SETBIT(0),
        //O_SUSPENDED     = SETBIT(1),
    };

};

struct sync_data_s
{
    active_protocol_data_s data;
    ts::str_c description; // utf8
    ts::str_c description_t; // utf8
    ts::str_c icon;
    ts::flags32_s flags;
    float volume = 1.0f;
    int dsp_flags = 0;
    contact_online_state_e manual_cos = COS_ONLINE;
    cmd_result_e current_state;
};

class active_protocol_c : public ts::safe_object
{
    GM_RECEIVER( active_protocol_c, ISOGM_PROFILE_TABLE_SAVED );
    GM_RECEIVER( active_protocol_c, ISOGM_MESSAGE);
    GM_RECEIVER( active_protocol_c, ISOGM_CHANGED_SETTINGS);
    GM_RECEIVER( active_protocol_c, GM_HEARTBEAT);
    GM_RECEIVER( active_protocol_c, ISOGM_CMD_RESULT);
    GM_RECEIVER( active_protocol_c, GM_UI_EVENT);
    
    int id;
    int priority = 0;
    int features = 0;
    int conn_features = 0;
    s3::Format audio_fmt;
    isotoxin_ipc_s * volatile ipcp = nullptr;
    spinlock::syncvar< sync_data_s > syncdata;
    ts::Time lastconfig;
    ts::Time typingtime = ts::Time::past();
    int typingsendcontact = 0;

    time_t last_backup_time = 0;

    fmt_converter_s cvt;
    long lbsync = 0;
    ts::pointers_t<data_header_s,0> locked_bufs;

    static const ts::flags32_s::BITS F_DIP                  = SETBIT(0);
    static const ts::flags32_s::BITS F_WORKER               = SETBIT(1);
    static const ts::flags32_s::BITS F_CONFIG_OK            = SETBIT(2);
    static const ts::flags32_s::BITS F_CONFIG_FAIL          = SETBIT(3);
    static const ts::flags32_s::BITS F_SAVE_REQUEST         = SETBIT(4);
    static const ts::flags32_s::BITS F_CONFIG_UPDATED       = SETBIT(5);
    static const ts::flags32_s::BITS F_CFGSAVE_CHECKER      = SETBIT(6);
    static const ts::flags32_s::BITS F_CONFIGURABLE_RCVD    = SETBIT(7);
    static const ts::flags32_s::BITS F_ONLINE_SWITCH        = SETBIT(8);
    static const ts::flags32_s::BITS F_SET_PROTO_OK         = SETBIT(9);
    static const ts::flags32_s::BITS F_CURRENT_ONLINE       = SETBIT(10);
    static const ts::flags32_s::BITS F_WORKER_STOPED        = SETBIT(11);

    struct icon_s
    {
        const ts::bitmap_c *bmp;
        icon_type_e icot;
    };

    ts::array_inplace_t<icon_s, 1> icons_cache; // FREE MEMORY

    bool cmdhandler(ipcr r);
    bool tick();
    static DWORD WINAPI worker(LPVOID);
    void worker();
#if defined _DEBUG || defined _CRASH_HANDLER
    void worker_check();
#endif
    bool check_die(RID, GUIPARAM);
    bool check_save(RID, GUIPARAM);
    void save_config( const ts::blob_c &cfg );
    void run();

    void push_debug_settings();

public:
    active_protocol_c(int id, const active_protocol_data_s &pd);
    ~active_protocol_c();

    void once_per_5sec_tick();

    void unlock_video_frame( incoming_video_frame_s *f );

    const ts::str_c &get_desc() const {return syncdata.lock_read()().description;};
    const ts::str_c &get_desc_t() const {return syncdata.lock_read()().description_t;};
    const ts::str_c &get_name() const {return syncdata.lock_read()().data.name;};
    const ts::str_c &get_tag() const { return syncdata.lock_read()().data.tag; };
    int get_features() const {return features; }
    int get_conn_features() const { return conn_features; }
    int get_priority() const {return priority; }

    const ts::str_c &get_uname() const {return syncdata.lock_read()().data.user_name;};
    const ts::str_c &get_ustatusmsg() const {return syncdata.lock_read()().data.user_statusmsg;};
    
    configurable_s get_configurable() const { return syncdata.lock_read()().data.configurable; };
    void set_configurable( const configurable_s &c, bool force_send = false );

    const s3::Format& defaudio() const {return audio_fmt;}

    const ts::bitmap_c &get_icon(int sz, icon_type_e icot);

    void set_current_online(bool oflg) { syncdata.lock_write()().flags.init(F_CURRENT_ONLINE, oflg); }
    bool is_current_online() const { return syncdata.lock_read()().flags.is(F_CURRENT_ONLINE); }
    cmd_result_e get_current_state() const { return syncdata.lock_read()().current_state; }

    int sort_factor() const { return syncdata.lock_read()().data.sort_factor; }
    void set_sortfactor(int sf);

    void set_avatar(contact_c *); // self avatar to self contact
    void set_avatar( const ts::blob_c &ava ); // avatar for this protocol
    void set_ostate(contact_online_state_e _cos);

    void save_config(bool wait);
    void stop_and_die(bool wait_worker_end = false);
    int getid() const {return id;}
    bool is_new() const { return id < 0 && !is_dip(); }
    bool is_dip() const { return syncdata.lock_read()().flags.is(F_DIP); }

    bool is_autoconnect() const { return 0 != (syncdata.lock_read()().data.options & active_protocol_data_s::O_AUTOCONNECT); }
    void set_autoconnect( bool v );

    void del_message( uint64 utag );

    void join_group_chat(int gid, int cid);
    void rename_group_chat(int gid, const ts::str_c &groupname);
    void add_group_chat( const ts::str_c &groupname, bool persistent );
    void resend_request( int cid, const ts::str_c &msg_utf8 );
    void add_contact( const ts::str_c& pub_id, const ts::str_c &msg_utf8 );
    void del_contact(int cid);
    void accept(int cid);
    void reject(int cid);

    void apply_encoding_settings(); // should be called before enabling video or during video call (to change current settings)
    void accept_call(int cid);
    void send_video_frame(int cid, const ts::bmpcore_exbody_s &eb);
    void send_audio(int cid, const s3::Format &fmt, const void *data, int size);
    void call(int cid, int seconds);
    void stop_call(int cid);
    void set_stream_options(int cid, int so, const ts::ivec2 &vr); // tell to proto/other peer about recomended video resolution (if I see video in 320x240, why you send 640x480?)

    void file_resume(uint64 utag, uint64 offset);
    void file_control(uint64 utag, file_control_e fctl);
    void send_file(int cid, uint64 utag, const ts::wstr_c &filename, uint64 filesize);
    void file_portion(uint64 utag, uint64 offset, const void *data, int sz);

    void avatar_data_request(int cid);

    void typing(int cid);
    
    void export_data();

};