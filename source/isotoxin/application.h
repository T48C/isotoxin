#pragma once

#define IDI_ICON_APP IDI_ICON_ONLINE

#define BUTTON_FACE_PRELOADED( face ) (GET_BUTTON_FACE) ([]()->button_desc_s * { return g_app->preloaded_stuff().face; } )
#define GET_FONT( face ) g_app->preloaded_stuff().face
#define GET_THEME_VALUE( name ) g_app->preloaded_stuff().name

#define HOTKEY_TOGGLE_SEARCH_BAR SSK_F, gui_c::casw_ctrl
#define HOTKEY_TOGGLE_TAGFILTER_BAR SSK_T, gui_c::casw_ctrl
#define HOTKEY_TOGGLE_NEW_CONNECTION_BAR SSK_N, gui_c::casw_ctrl

//-V:preloaded_stuff():807
//-V:GET_FONT:807
//-V:GET_THEME_VALUE:807

struct preloaded_stuff_s
{
    const ts::font_desc_c *font_conv_name = &ts::g_default_text_font;
    const ts::font_desc_c *font_conv_text = &ts::g_default_text_font;
    const ts::font_desc_c *font_conv_time = &ts::g_default_text_font;
    int contactheight = 55;
    int mecontactheight = 60;
    int minprotowidth = 100;
    int protoiconsize = 10;
    ts::ivec2 achtung_shift = ts::ivec2(0);

    ts::TSCOLOR common_bg_color = 0xffffffff;
    ts::TSCOLOR appname_color = ts::ARGB(0, 50, 0);
    ts::TSCOLOR found_mark_color = ts::ARGB(50, 50, 0);
    ts::TSCOLOR found_mark_bg_color = ts::ARGB(255, 100, 255);
    ts::TSCOLOR achtung_content_color = ts::ARGB(0, 0, 0);
    ts::TSCOLOR state_online_color = ts::ARGB(0, 255, 0);
    ts::TSCOLOR state_away_color = ts::ARGB(255, 255, 0);
    ts::TSCOLOR state_dnd_color = ts::ARGB(255, 0, 0);

    const theme_image_s* icon[contact_gender_count];
    const theme_image_s* groupchat = nullptr;
    const theme_image_s* nokeeph = nullptr;
    const theme_image_s* achtung_bg = nullptr;
    const theme_image_s* invite_send = nullptr;
    const theme_image_s* invite_recv = nullptr;
    const theme_image_s* invite_rej = nullptr;
    const theme_image_s* online[contact_online_state_check];
    const theme_image_s* offline = nullptr;
    const theme_image_s* online_some = nullptr;
    
    ts::shared_ptr<button_desc_s> callb;
    ts::shared_ptr<button_desc_s> fileb;

    ts::shared_ptr<button_desc_s> editb;
    ts::shared_ptr<button_desc_s> confirmb;
    ts::shared_ptr<button_desc_s> cancelb;

    ts::shared_ptr<button_desc_s> exploreb;
    ts::shared_ptr<button_desc_s> pauseb;
    ts::shared_ptr<button_desc_s> unpauseb;
    ts::shared_ptr<button_desc_s> breakb;
    
    
    ts::shared_ptr<button_desc_s> smile;

    void update_fonts();
    void reload();
};

enum autoupdate_beh_e
{
    AUB_NO,
    AUB_ONLY_CHECK,
    AUB_DOWNLOAD,

    AUB_DEFAULT = AUB_NO,
};

struct autoupdate_params_s
{
    ts::str_c ver;
    ts::wstr_c path;
    ts::str_c aurl;
    ts::str_c proxy_addr;
    int proxy_type = 0;
    autoupdate_beh_e autoupdate = AUB_NO;
    bool in_updater = false;
    bool in_progress = false;
    bool downloaded = false;
};

struct file_transfer_s;
struct query_task_s : public ts::task_c
{
    struct job_s
    {
        DUMMY(job_s);
        uint64 offset = 0xFFFFFFFFFFFFFFFFull;
        int sz = 0;
        job_s() {}
    };

    enum rslt_e
    {
        rslt_inprogress,
        rslt_idle,
        rslt_kill,
        rslt_ok,
    };

    struct sync_s
    {
        file_transfer_s *ftr = nullptr;
        job_s current_job;
        ts::array_inplace_t<job_s, 1> jobarray;
        rslt_e rslt = rslt_inprogress;
    };

    spinlock::syncvar<sync_s> sync;

    query_task_s(file_transfer_s *ftr);
    ~query_task_s();
    /*virtual*/ int iterate(int pass) override;
    /*virtual*/ void done(bool canceled) override;
    /*virtual*/ void result() override;
};

struct file_transfer_s : public unfinished_file_transfer_s
{
    MOVABLE(true);

    static const int BPSSV_WAIT_FOR_ACCEPT = -3;
    static const int BPSSV_PAUSED_BY_REMOTE = -2;
    static const int BPSSV_PAUSED_BY_ME = -1;
    static const int BPSSV_ALLOW_CALC = 0;


    query_task_s *query_task = nullptr;

    struct data_s
    {
        //uint64 offset = 0;
        uint64 progrez = 0;
        HANDLE handle = nullptr;
        double notfreq = 1.0;
        LARGE_INTEGER prevt;
        int bytes_per_sec = BPSSV_ALLOW_CALC;
        float upduitime = 0;

        struct range_s
        {
            uint64 offset0;
            uint64 offset1;
        };
        ts::tbuf0_t<range_s> transfered;
        void tr(uint64 _offset0, uint64 _offset1);
        uint64 trsz() const
        {
            uint64 sz = 0;
            for (const range_s &r : transfered)
                sz += r.offset1 - r.offset0;
            return sz;
        }

        float deltatime(bool updateprevt, int addseconds = 0)
        {
            LARGE_INTEGER cur;
            QueryPerformanceCounter(&cur);
            float dt = (float)((double)(cur.QuadPart - prevt.QuadPart) * notfreq);
            if (updateprevt)
            {
                prevt = cur;
                if (addseconds)
                    prevt.QuadPart += (int64)((double)addseconds / notfreq);
            }
            return dt;
        }

    };

    spinlock::syncvar<data_s> data;

    HANDLE file_handle() const { return data.lock_read()().handle; }
    //uint64 get_offset() const { return data.lock_read()().offset; }

    bool accepted = false; // prepare_fn called - file receive accepted // used only for receive
    bool update_item = false;

    file_transfer_s();
    ~file_transfer_s();

    bool auto_confirm();

    int progress(int &bytes_per_sec) const;
    void upd_message_item(bool force);


    void upload_accepted();
    void resume();
    void prepare_fn( const ts::wstr_c &path_with_fn, bool overwrite );
    void kill( file_control_e fctl = FIC_BREAK );
    void save( uint64 offset, const ts::buf0_c&data );
    void query( uint64 offset, int sz );
    void pause_by_remote( bool p );
    void pause_by_me( bool p );
    bool is_active() const 
    {
        auto rdata = data.lock_read();
        return rdata().bytes_per_sec == BPSSV_WAIT_FOR_ACCEPT || (const_cast<data_s *>(&rdata())->deltatime(false)) < 60; /* last activity in 60 sec */
    }
    bool confirm_required() const;
};

struct av_contact_s
{
    time_t starttime;
    ts::shared_ptr<contact_root_c> c;
    enum state_e
    {
        AV_NONE,
        AV_RINGING,
        AV_INPROGRESS,
    };
    int so = SO_SENDING_AUDIO | SO_RECEIVING_AUDIO | SO_RECEIVING_VIDEO; // default stream options
    int remote_so = 0;
    ts::ivec2 sosz = ts::ivec2(0);
    ts::ivec2 remote_sosz = ts::ivec2(0);
    ts::ivec2 prev_video_size = ts::ivec2(0);
    vsb_descriptor_s currentvsb;
    UNIQUE_PTR(vsb_c) vsb;
    active_protocol_c *ap4video = nullptr;
    int videocid = 0;

    typedef gm_redirect_s<ISOGM_PEER_STREAM_OPTIONS> OPTIONS_HANDLER;
    UNIQUE_PTR(OPTIONS_HANDLER) options_handler;

    bool ohandler( gmsg<ISOGM_PEER_STREAM_OPTIONS> &so );

    unsigned state : 4;
    unsigned inactive : 1; // true - selected other av contact
    unsigned dirty_cam_size : 1;

    av_contact_s(contact_root_c *c, state_e st);

    void on_frame_ready( const ts::bmpcore_exbody_s &ebm );
    void camera_tick();

    bool is_mic_off() const { return 0 == ( cur_so() & SO_SENDING_AUDIO ); }
    bool is_mic_on() const { return 0 != ( cur_so() & SO_SENDING_AUDIO ); }

    bool is_speaker_off() const { return 0 == (cur_so() & SO_RECEIVING_AUDIO); }
    bool is_speaker_on() const { return 0 != (cur_so() & SO_RECEIVING_AUDIO); }

    bool is_video_show() const { return is_receive_video() && 0 != ( remote_so & SO_SENDING_VIDEO ); }
    bool is_receive_video() const { return 0 != (cur_so() & SO_RECEIVING_VIDEO); }
    bool is_camera_on() const { return 0 != (cur_so() & SO_SENDING_VIDEO); }

    bool b_mic_switch(RID, GUIPARAM p);
    bool b_speaker_switch(RID, GUIPARAM p);

    void update_btn_face_camera(gui_button_c &btn);

    void update_speaker();

    void set_recv_video(bool allow_recv);
    void set_inactive( bool inactive );
    void set_so_audio( bool inactive, bool enable_mic, bool enable_speaker );
    void camera( bool on );
    
    void camera_switch();
    void mic_off();
    void mic_switch();
    void speaker_switch();
    void set_video_res( const ts::ivec2 &vsz );
    void send_so();
    int cur_so() const { return inactive ? ( so & ~(SO_SENDING_AUDIO|SO_RECEIVING_AUDIO|SO_RECEIVING_VIDEO) ) : so; }

    vsb_c *createcam();
};


class application_c : public gui_c, public sound_capture_handler_c
{
    bool b_customize(RID r, GUIPARAM param);

    ts::tbuf_t<s3::Format> avformats;
    /*virtual*/ void datahandler(const void *data, int size) override;
    /*virtual*/ const s3::Format *formats(int &count) override;

public:
    static const ts::flags32_s::BITS PEF_RECREATE_CTLS_CL = PEF_FREEBITSTART << 0;
    static const ts::flags32_s::BITS PEF_RECREATE_CTLS_MSG = PEF_FREEBITSTART << 1;
    static const ts::flags32_s::BITS PEF_UPDATE_BUTTONS_HEAD = PEF_FREEBITSTART << 2;
    static const ts::flags32_s::BITS PEF_UPDATE_BUTTONS_MSG = PEF_FREEBITSTART << 3;
    static const ts::flags32_s::BITS PEF_SHOW_HIDE_EDITOR = PEF_FREEBITSTART << 4;
    
    
    static const ts::flags32_s::BITS PEF_APP = (ts::flags32_s::BITS)(-1) & (~(PEF_FREEBITSTART-1));
    

    /*virtual*/ HICON app_icon(bool for_tray) override;
    /*virtual*/ void app_setup_custom_button(bcreate_s & bcr) override
    {
        if (bcr.tag == CBT_APPCUSTOM)
        {
            bcr.face = BUTTON_FACE(customize);
            bcr.handler = DELEGATE(this, b_customize);
            bcr.tooltip = TOOLTIP(TTT("Settings",2));
        }
    }
    /*virtual*/ bool app_custom_button_state(int tag, int &shiftleft) override
    {
        if (tag == CBT_APPCUSTOM)
            shiftleft += 5;

        return true;
    }

    /*virtual*/ void app_prepare_text_for_copy(ts::str_c &text_utf8) override;

    /*virtual*/ ts::wsptr app_loclabel(loc_label_e ll);

    /*virtual*/ void app_notification_icon_action(naction_e act, RID iconowner) override;
    /*virtual*/ void app_fix_sleep_value(int &sleep_ms) override;
    /*virtual*/ void app_5second_event() override;
    /*virtual*/ void app_loop_event() override;
    /*virtual*/ void app_b_minimize(RID main) override;
    /*virtual*/ void app_b_close(RID main) override;
    /*virtual*/ void app_path_expand_env(ts::wstr_c &path) override;
    /*virtual*/ void do_post_effect() override;
    /*virtual*/ void app_font_par(const ts::str_c&, ts::font_params_s&fprm) override;



    ///////////// application_c itself

    RID main;

    unsigned F_INITIALIZATION : 1;
    unsigned F_NEWVERSION : 1;
    unsigned F_NONEWVERSION : 1;
    unsigned F_UNREADICON : 1;
    unsigned F_NEED_BLINK_ICON : 1;
    unsigned F_BLINKING_FLAG : 1;
    unsigned F_BLINKING_ICON : 1;
    unsigned F_SETNOTIFYICON : 1; // once
    unsigned F_OFFLINE_ICON : 1;
    unsigned F_ALLOW_AUTOUPDATE : 1;
    unsigned F_PROTOSORTCHANGED : 1;
    unsigned F_READONLY_MODE : 1;
    unsigned F_READONLY_MODE_WARN : 1;
    unsigned F_MODAL_ENTER_PASSWORD : 1;
    unsigned F_TYPING : 1; // any typing
    unsigned F_CAPTURE_AUDIO_TASK : 1;
    unsigned F_CAPTURING : 1;
    unsigned F_SHOW_CONTACTS_IDS : 1;

    SIMPLE_SYSTEM_EVENT_RECEIVER (application_c, SEV_EXIT);
    SIMPLE_SYSTEM_EVENT_RECEIVER (application_c, SEV_INIT);

    GM_RECEIVER( application_c, ISOGM_PROFILE_TABLE_SAVED );
    GM_RECEIVER( application_c, ISOGM_CHANGED_SETTINGS );
    GM_RECEIVER( application_c, GM_UI_EVENT );
    GM_RECEIVER( application_c, ISOGM_DELIVERED );
    GM_RECEIVER( application_c, ISOGM_EXPORT_PROTO_DATA );
    
    ts::array_inplace_t<av_contact_s,0> m_avcontacts;

    int get_avinprogresscount() const;
    int get_avringcount() const;
    av_contact_s & get_avcontact( contact_root_c *c, av_contact_s::state_e st );
    av_contact_s * find_avcontact_inprogress( contact_root_c *c );
    void del_avcontact(contact_root_c *c);
    template<typename R> void iterate_avcontacts( const R &r ) const { for( const av_contact_s &avc : m_avcontacts) r(avc); }
    template<typename R> void iterate_avcontacts( const R &r ) { for( av_contact_s &avc : m_avcontacts) r(avc); }
    void stop_all_av();

    mediasystem_c m_mediasystem;
    ts::task_executor_c m_tasks_executor;

    ts::hashmap_t<int, ts::wstr_c> m_locale;
    ts::hashmap_t<SLANGID, ts::wstr_c> m_locale_lng;
    SLANGID m_locale_tag;

    preloaded_stuff_s m_preloaded_stuff;

    ts::array_del_t<file_transfer_s, 2> m_files;

    struct blinking_reason_s
    {
        time_t last_update = now();
        contact_key_s historian;
        ts::Time nextblink = ts::Time::undefined();
        int unread_count = 0;
        ts::flags32_s flags;
        ts::tbuf_t<uint64> ftags_request, ftags_progress;

        static const ts::flags32_s::BITS F_BLINKING_FLAG = SETBIT(0);
        static const ts::flags32_s::BITS F_CONTACT_BLINKING = SETBIT(1);
        static const ts::flags32_s::BITS F_RINGTONE = SETBIT(2);
        static const ts::flags32_s::BITS F_INVITE_FRIEND = SETBIT(3);
        static const ts::flags32_s::BITS F_RECALC_UNREAD = SETBIT(4);
        static const ts::flags32_s::BITS F_NEW_VERSION = SETBIT(5);
        static const ts::flags32_s::BITS F_REDRAW = SETBIT(6);

        void do_recalc_unread_now();
        bool tick();

        bool get_blinking() const {return flags.is(F_BLINKING_FLAG);}

        bool is_blank() const
        {
            if (contacts().find(historian) == nullptr) return true;
            return unread_count == 0 && ftags_request.count() == 0 && ftags_progress.count() == 0 && (flags.__bits & ~(F_CONTACT_BLINKING|F_BLINKING_FLAG)) == 0;
        }
        bool notification_icon_need_blink() const
        {
            return flags.is(F_RINGTONE | F_INVITE_FRIEND) || unread_count > 0 || is_file_download_request();
        }
        bool contact_need_blink() const
        {
            return notification_icon_need_blink() || is_file_download_process();
        }

        void ringtone(bool f = true)
        {
            if (flags.is(F_RINGTONE) != f)
            {
                flags.init(F_RINGTONE, f);
                flags.set(F_REDRAW);
            }
        }
        void friend_invite(bool f = true)
        {
            if (flags.is(F_INVITE_FRIEND) != f)
            {
                flags.init(F_INVITE_FRIEND, f);
                flags.set(F_REDRAW);
            }
        }
        bool is_file_download() const { return is_file_download_request() || is_file_download_process(); }
        bool is_file_download_request() const { return ftags_request.count() > 0; }
        bool is_file_download_process() const { return ftags_progress.count() > 0; }
        void file_download_request_add( uint64 ftag )
        {
            bool dirty = ftags_progress.find_remove_fast(ftag);
            int oldc = ftags_request.count();
            ftags_request.set(ftag);
            if (dirty || oldc != ftags_request.count())
                flags.set(F_REDRAW);
        }
        void file_download_progress_add( uint64 ftag )
        {
            bool dirty = ftags_request.find_remove_fast(ftag);
            int oldc = ftags_progress.count();
            ftags_progress.set(ftag);
            if (dirty || oldc != ftags_progress.count())
                flags.set(F_REDRAW);
        }
        void file_download_remove( uint64 ftag )
        {
            bool was_f = is_file_download();
            if (!ftag)
                ftags_request.clear(), ftags_progress.clear();
            else
                ftags_request.find_remove_fast(ftag), ftags_progress.find_remove_fast(ftag);
            if (was_f)
                flags.set(F_REDRAW);
        }
        void new_version(bool f = true)
        {
            if (flags.is(F_NEW_VERSION) != f)
            {
                flags.init(F_NEW_VERSION, f);
                flags.set(F_REDRAW);
            }
        }

        void recalc_unread()
        {
            flags.set(F_RECALC_UNREAD);
        }

        void up_unread()
        {
            last_update = now();
            ++unread_count;
            flags.set(F_REDRAW);
            g_app->F_SETNOTIFYICON = true;
        }

        void set_unread(int unread)
        {
            if (unread != unread_count)
            {
                flags.set(F_REDRAW);
                unread_count = unread;
                g_app->F_SETNOTIFYICON = true;
            }
        }

    };

    ts::array_inplace_t<blinking_reason_s,2> m_blink_reasons;
    ts::tbuf_t<contact_key_s> m_locked_recalc_unread;
    
    sound_capture_handler_c *m_currentsc = nullptr;
    ts::pointers_t<sound_capture_handler_c, 0> m_scaptures;

    struct send_queue_s
    {
        ts::Time last_try_send_time = ts::Time::current();
        contact_key_s receiver; // metacontact
        ts::array_inplace_t<post_s, 1> queue; // sorted by time
    };

    ts::array_del_t<send_queue_s, 1> m_undelivered;

public:
    bool b_send_message(RID r, GUIPARAM param);
    bool flash_notification_icon(RID r = RID(), GUIPARAM param = nullptr);
    bool flashingicon() const {return F_UNREADICON;};
public:

    ts::safe_ptr<gui_contact_item_c> active_contact_item;
    vsb_display_ptr_c current_video_display;

    time_t autoupdate_next;
    ts::ivec2 download_progress = ts::ivec2(0);

    found_stuff_s *found_items = nullptr;

    class splchk_c
    {
        mutable long sync = 0;
        enum
        {
            EMPTY,
            LOADING,
            READY,
        } state = EMPTY;
        void *busy = nullptr;
        bool unload_request = false;
        ts::array_del_t< Hunspell, 0 > spellcheckers;
    public:

        enum lock_rslt_e
        {
            LOCK_OK,
            LOCK_BUSY,
            LOCK_EMPTY
        };

        bool is_enabled() const
        {
            spinlock::auto_simple_lock l(sync);
            return state != EMPTY && !unload_request;
        }

        void load();
        void unload();
        void set_spellcheckers(ts::array_del_t< Hunspell, 0 > &&sa);
        void check(ts::astrings_c &&checkwords, spellchecker_s *rsltrcvr);
        lock_rslt_e lock( void *prm );
        bool unlock( void *prm );
        bool check_one( const ts::str_c &w, ts::astrings_c &suggestions );

    } spellchecker;

	application_c( const ts::wchar * cmdl );
	~application_c();


    void apply_debug_options();

    static ts::str_c get_downloaded_ver();
    bool b_update_ver(RID, GUIPARAM);
    bool b_restart(RID, GUIPARAM);
    bool b_install(RID, GUIPARAM);
    
    void newversion(bool nv) { F_NEWVERSION = nv; };
    void nonewversion(bool nv) { F_NONEWVERSION = nv; };
    bool newversion() const {return F_NEWVERSION;}
    bool nonewversion() const {return F_NONEWVERSION;}

    static ts::str_c appver();
    void set_notification_icon();
    bool is_inactive(bool do_incoming_message_stuff);

    void reload_fonts();
    bool load_theme( const ts::wsptr&thn, bool summon_ch_signal = true);
    ts::bitmap_c &prepareimageplace(const ts::wsptr &name)
    {
        return get_theme().prepareimageplace(name);
    }
    void clearimageplace(const ts::wsptr &name)
    {
        return get_theme().clearimageplace(name);
    }

    const SLANGID &current_lang() const {return m_locale_tag;};
    void load_locale( const SLANGID& lng );
    const ts::wstr_c &label(int id) const
    {
        const ts::wstr_c *l = m_locale.get(id);
        if (l) return *l;
        return ts::make_dummy<ts::wstr_c>(true);
    }

    void summon_main_rect(bool minimize);
    void load_profile_and_summon_main_rect(bool minimize);
    preloaded_stuff_s &preloaded_stuff() {return m_preloaded_stuff;}

    void lock_recalc_unread( const contact_key_s &ck ) { m_locked_recalc_unread.set(ck); };
    blinking_reason_s &new_blink_reason(const contact_key_s &historian);
    void update_blink_reason(const contact_key_s &historian);
    blinking_reason_s *find_blink_reason(const contact_key_s &historian, bool skip_locked)
    { 
        for (blinking_reason_s &br : m_blink_reasons)
            if (br.historian == historian)
            {
                if (skip_locked && m_locked_recalc_unread.find_index(historian) >= 0)
                    return nullptr;
                return &br;
            }
        return nullptr;
    }
    bool present_unread_blink_reason() const
    {
        for (const blinking_reason_s &br : m_blink_reasons)
            if (br.unread_count > 0)
                return true;
        return false;
    }
    void select_last_unread_contact();

    void handle_sound_capture( const void *data, int size );
    void register_capture_handler( sound_capture_handler_c *h );
    void unregister_capture_handler( sound_capture_handler_c *h );
    void start_capture(sound_capture_handler_c *h);
    void stop_capture(sound_capture_handler_c *h);
    void capture_device_changed();
    void check_capture();
    bool capturing() const
    {
        if (F_CAPTURE_AUDIO_TASK) return false;
        return F_CAPTURING;
    }

    mediasystem_c &mediasystem() {return m_mediasystem;};

    void add_task( ts::task_c *t ) { m_tasks_executor.add(t); }

    void update_ringtone( contact_root_c *rt, bool play_stop_snd = true );
    av_contact_s * update_av( contact_root_c *avmc, bool activate, bool camera = false );


    template<typename R> void enum_file_transfers_by_historian( const contact_key_s &historian, R r )
    {
        for (file_transfer_s *ftr : m_files)
            if (ftr->historian == historian)
                r(*ftr);
    }
    bool present_file_transfer_by_historian(const contact_key_s &historian);
    bool present_file_transfer_by_sender(const contact_key_s &sender, bool accept_only_rquest);
    file_transfer_s *find_file_transfer(uint64 utag);
    file_transfer_s *find_file_transfer_by_msgutag(uint64 utag);
    file_transfer_s *register_file_transfer( const contact_key_s &historian, const contact_key_s &sender, uint64 utag, const ts::wstr_c &filename, uint64 filesize );
    void unregister_file_transfer(uint64 utag,bool disconnected);
    void cancel_file_transfers( const contact_key_s &historian ); // by historian

    void resend_undelivered_messages( const contact_key_s& rcv = contact_key_s() );
    void undelivered_message( const post_s &p );
    void kill_undelivered( uint64 utag );

    void set_status(contact_online_state_e cos_, bool manual);

    void recreate_ctls(bool cl, bool m)
    {
        if (cl) m_post_effect.set(PEF_RECREATE_CTLS_CL); 
        if (m) m_post_effect.set(PEF_RECREATE_CTLS_MSG);
    };
    void update_buttons_head() { m_post_effect.set(PEF_UPDATE_BUTTONS_HEAD); };
    void update_buttons_msg() { m_post_effect.set(PEF_UPDATE_BUTTONS_MSG); };
    void hide_show_messageeditor() { m_post_effect.set(PEF_SHOW_HIDE_EDITOR); };

};

extern application_c *g_app;

INLINE bool contact_root_c::match_tags(int bitags) const
{
    if (bitags & (1 << BIT_ALL)) return true;

    const ts::buf0_c &enabledbits = contacts().get_tags_bits();

    bool bitag = false;
    if (bitags & (1 << BIT_UNTAGGED))
    {
        if (!tags_bits.is_any_bit()) return true;
        bitag = true;
    }

    if (bitags & (1 << BIT_ONLINE))
    {
        contact_state_e cs = get_meta_state();
        if (CS_ONLINE == cs || contact_state_check == cs)
            return true;
        bitag = true;
    }

    if (g_app->find_blink_reason(getkey(), false) != nullptr)
        return true;

    if (enabledbits.size() == 0) return !bitag;
    return contacts().get_tags_bits().is_any_common_bit(tags_bits);
}

