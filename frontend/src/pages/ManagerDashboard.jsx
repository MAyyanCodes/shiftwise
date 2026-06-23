import { useState, useEffect } from 'react';
import { useAuth } from '../hooks/useAuth';
import api from '../utils/api';
import { Users, Clock, Umbrella, AlertCircle, TrendingUp, CheckCircle, XCircle } from 'lucide-react';
import { format } from 'date-fns';

function StatCard({ value, label, color = 'var(--accent)' }) {
  return (
    <div className="stat-card">
      <div className="stat-value" style={{ color }}>{value}</div>
      <div className="stat-label">{label}</div>
    </div>
  );
}

function PendingManagers({ onAction }) {
  const [pending, setPending] = useState([]);
  const [loading, setLoading] = useState(true);
  const [acting, setActing]   = useState(null); // id currently being approved/rejected

  const fetchPending = async () => {
    try {
      const res = await api.get('/auth/pending-managers');
      setPending(res.data);
    } catch (e) {
      console.error(e);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => { fetchPending(); }, []);

  const handleAction = async (id, action) => {
    setActing(id);
    try {
      await api.put(`/auth/${action}/${id}`);
      setPending(prev => prev.filter(m => m.id !== id));
      onAction(); // refresh parent dashboard stats
    } catch (e) {
      console.error(e);
    } finally {
      setActing(null);
    }
  };

  if (loading) return null;
  if (pending.length === 0) return null; // hide section entirely if nothing pending

  return (
    <div className="card" style={{ marginBottom: 20, border: '1px solid var(--warning)' }}>
      <div className="card-header">
        <h2 className="card-title">
          <span style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
            <AlertCircle size={16} color="var(--warning)" />
            Pending Manager Approvals
          </span>
        </h2>
        <span className="badge badge-pending">{pending.length} pending</span>
      </div>

      <div style={{ display: 'flex', flexDirection: 'column', gap: 10 }}>
        {pending.map(mgr => (
          <div key={mgr.id} style={{
            display: 'flex', alignItems: 'center', gap: 12,
            padding: '12px 14px', background: 'var(--bg-elevated)',
            borderRadius: 'var(--radius-sm)',
            borderLeft: '3px solid var(--warning)'
          }}>
            <div style={{
              width: 38, height: 38, borderRadius: '50%',
              background: 'var(--warning-dim)', color: 'var(--warning)',
              display: 'flex', alignItems: 'center', justifyContent: 'center',
              fontWeight: 700, fontSize: '0.8rem', flexShrink: 0
            }}>
              {mgr.first_name[0]}{mgr.last_name[0]}
            </div>

            <div style={{ flex: 1 }}>
              <div style={{ fontWeight: 600, fontSize: '0.875rem' }}>
                {mgr.first_name} {mgr.last_name}
              </div>
              <div style={{ fontSize: '0.72rem', color: 'var(--text-muted)' }}>
                @{mgr.username} · {mgr.email}
              </div>
            </div>

            <div style={{ display: 'flex', gap: 8, flexShrink: 0 }}>
              <button
                className="btn btn-sm"
                style={{
                  background: 'var(--success-dim)', color: 'var(--success)',
                  border: '1px solid var(--success)', display: 'flex',
                  alignItems: 'center', gap: 4, padding: '6px 12px'
                }}
                disabled={acting === mgr.id}
                onClick={() => handleAction(mgr.id, 'approve')}
              >
                {acting === mgr.id
                  ? <div className="spinner" style={{ width: 12, height: 12 }} />
                  : <><CheckCircle size={13} /> Approve</>
                }
              </button>
              <button
                className="btn btn-sm"
                style={{
                  background: 'var(--danger-dim)', color: 'var(--danger)',
                  border: '1px solid var(--danger)', display: 'flex',
                  alignItems: 'center', gap: 4, padding: '6px 12px'
                }}
                disabled={acting === mgr.id}
                onClick={() => handleAction(mgr.id, 'reject')}
              >
                {acting === mgr.id
                  ? <div className="spinner" style={{ width: 12, height: 12 }} />
                  : <><XCircle size={13} /> Reject</>
                }
              </button>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}

function WorkingNow({ employees }) {
  return (
    <div className="card">
      <div className="card-header">
        <h2 className="card-title">
          <span style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
            <Clock size={16} color="var(--success)" />
            Currently Working
          </span>
        </h2>
        <span className="badge badge-approved">{employees.length} active</span>
      </div>
      {employees.length === 0 ? (
        <div className="empty-state" style={{ padding: '24px 0' }}>
          <p>No one is on shift right now</p>
        </div>
      ) : (
        <div style={{ display: 'flex', flexDirection: 'column', gap: 8 }}>
          {employees.map(emp => (
            <div key={emp.id} style={{
              display: 'flex', alignItems: 'center', gap: 12,
              padding: '10px 12px', background: 'var(--bg-elevated)',
              borderRadius: 'var(--radius-sm)'
            }}>
              <div style={{
                width: 36, height: 36, borderRadius: '50%',
                background: 'var(--success-dim)', color: 'var(--success)',
                display: 'flex', alignItems: 'center', justifyContent: 'center',
                fontWeight: 700, fontSize: '0.8rem', flexShrink: 0
              }}>
                {emp.first_name[0]}{emp.last_name[0]}
              </div>
              <div style={{ flex: 1 }}>
                <div style={{ fontWeight: 600, fontSize: '0.875rem' }}>
                  {emp.first_name} {emp.last_name}
                </div>
                <div style={{ fontSize: '0.72rem', color: 'var(--text-muted)' }}>
                  {emp.position || 'No position'} · {emp.start_time} – {emp.end_time}
                </div>
              </div>
              <span className="badge badge-approved">On shift</span>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}

function OnLeave({ employees }) {
  return (
    <div className="card">
      <div className="card-header">
        <h2 className="card-title">
          <span style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
            <Umbrella size={16} color="var(--warning)" />
            On Leave Today
          </span>
        </h2>
        <span className="badge badge-pending">{employees.length} away</span>
      </div>
      {employees.length === 0 ? (
        <div className="empty-state" style={{ padding: '24px 0' }}>
          <p>No one is on leave today</p>
        </div>
      ) : (
        <div style={{ display: 'flex', flexDirection: 'column', gap: 8 }}>
          {employees.map(emp => (
            <div key={emp.id} style={{
              display: 'flex', alignItems: 'center', gap: 12,
              padding: '10px 12px', background: 'var(--bg-elevated)',
              borderRadius: 'var(--radius-sm)'
            }}>
              <div style={{
                width: 36, height: 36, borderRadius: '50%',
                background: 'var(--warning-dim)', color: 'var(--warning)',
                display: 'flex', alignItems: 'center', justifyContent: 'center',
                fontWeight: 700, fontSize: '0.8rem', flexShrink: 0
              }}>
                {emp.first_name[0]}{emp.last_name[0]}
              </div>
              <div style={{ flex: 1 }}>
                <div style={{ fontWeight: 600, fontSize: '0.875rem' }}>
                  {emp.first_name} {emp.last_name}
                </div>
                <div style={{ fontSize: '0.72rem', color: 'var(--text-muted)' }}>
                  {emp.leave_type} · until {emp.end_date}
                </div>
              </div>
              <span className="badge badge-pending">On leave</span>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}

function AnnouncementWall({ announcements, isManager, onPost }) {
  const [title, setTitle]     = useState('');
  const [body, setBody]       = useState('');
  const [priority, setPri]    = useState('normal');
  const [posting, setPosting] = useState(false);

  const handlePost = async () => {
    if (!title || !body) return;
    setPosting(true);
    await onPost({ title, body, priority });
    setTitle(''); setBody(''); setPri('normal');
    setPosting(false);
  };

  return (
    <div className="card">
      <div className="card-header">
        <h2 className="card-title">Announcement Wall</h2>
      </div>

      {isManager && (
        <div style={{
          background: 'var(--bg-elevated)', borderRadius: 'var(--radius-md)',
          padding: 16, marginBottom: 20
        }}>
          <input
            className="form-input"
            placeholder="Announcement title..."
            value={title}
            onChange={e => setTitle(e.target.value)}
            style={{ marginBottom: 8 }}
          />
          <textarea
            className="form-input"
            placeholder="Write your message here..."
            value={body}
            onChange={e => setBody(e.target.value)}
            rows={3}
            style={{ resize: 'vertical', marginBottom: 8 }}
          />
          <div style={{ display: 'flex', alignItems: 'center', gap: 8, justifyContent: 'space-between' }}>
            <select className="form-input" value={priority} onChange={e => setPri(e.target.value)}
              style={{ width: 'auto' }}>
              <option value="normal">Normal</option>
              <option value="high">High</option>
              <option value="urgent">Urgent</option>
            </select>
            <button
              className="btn btn-primary btn-sm"
              onClick={handlePost}
              disabled={posting || !title || !body}
            >
              {posting ? <div className="spinner" style={{ width: 14, height: 14 }} /> : 'Post'}
            </button>
          </div>
        </div>
      )}

      <div>
        {announcements.length === 0 ? (
          <div className="empty-state" style={{ padding: '24px 0' }}>
            <p>No announcements yet</p>
          </div>
        ) : (
          announcements.map(a => (
            <div key={a.id} className={`announcement-card ${a.priority}`}>
              <div style={{ display: 'flex', alignItems: 'flex-start', justifyContent: 'space-between', gap: 8 }}>
                <div className="announcement-title">{a.title}</div>
                <span className={`badge badge-${a.priority}`}>{a.priority}</span>
              </div>
              <div className="announcement-body">{a.body}</div>
              <div className="announcement-meta">
                <span>Posted by {a.posted_by}</span>
                <span>{format(new Date(a.created_at), 'MMM d, yyyy')}</span>
              </div>
            </div>
          ))
        )}
      </div>
    </div>
  );
}

export default function ManagerDashboard() {
  const { user } = useAuth();
  const [data, setData]         = useState(null);
  const [announcements, setAnn] = useState([]);
  const [loading, setLoading]   = useState(true);

  const fetchAll = async () => {
    try {
      const [dash, ann] = await Promise.all([
        api.get('/dashboard/manager'),
        api.get('/announcements')
      ]);
      setData(dash.data);
      setAnn(ann.data);
    } catch (e) {
      console.error(e);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => { fetchAll(); }, []);

  const handlePost = async (payload) => {
    await api.post('/announcements', payload);
    const res = await api.get('/announcements');
    setAnn(res.data);
  };

  if (loading) return (
    <div style={{ display: 'flex', justifyContent: 'center', paddingTop: 80 }}>
      <div className="spinner" style={{ width: 32, height: 32 }} />
    </div>
  );

  return (
    <div>
      <div className="page-header">
        <div>
          <h1 className="page-title">Good morning, {user?.first_name} 👋</h1>
          <p className="page-subtitle">{format(new Date(), 'EEEE, MMMM d yyyy')}</p>
        </div>
      </div>

      {/* ADMIN APPROVAL PANEL — only shows if there are pending managers */}
      <PendingManagers onAction={fetchAll} />

      <div className="stat-grid">
        <StatCard value={data?.stats?.total_employees ?? 0}  label="Total Employees" />
        <StatCard value={data?.stats?.shifts_today ?? 0}     label="Shifts Today"    color="var(--success)" />
        <StatCard value={data?.stats?.on_leave_today ?? data?.on_leave_today?.length ?? 0} label="On Leave" color="var(--warning)" />
        <StatCard value={data?.stats?.pending_leaves ?? 0}   label="Pending Leaves"  color="var(--danger)"  />
      </div>

      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 20, marginBottom: 20 }}>
        <WorkingNow employees={data?.working_now ?? []} />
        <OnLeave    employees={data?.on_leave_today ?? []} />
      </div>

      <AnnouncementWall
        announcements={announcements}
        isManager={true}
        onPost={handlePost}
      />
    </div>
  );
}
