#include "acl_stdafx.hpp"
#include "acl_cpp/redis/redis_client_cluster.hpp"
#include "acl_cpp/redis/redis.hpp"
#include "acl_cpp/session/redis_session.hpp"

namespace acl
{

redis_session::redis_session(redis_client_cluster& cluster, size_t max_conns,
	time_t ttl /* = 0 */, const char* sid /* = NULL */)
	: session(ttl, sid)
	, cluster_(cluster)
{
	command_ = NEW redis;
	command_->set_cluster(&cluster_, max_conns == 0 ? 128 : max_conns);
}

redis_session::~redis_session()
{
	delete command_;
	std::map<string, session_string*>::iterator it;
	for (it = buffers_.begin(); it != buffers_.end(); ++it)
		delete it->second;
}

bool redis_session::set(const char* name, const char* value)
{
	return set(name, value, strlen(value));
}

bool redis_session::set(const char* name, const void* value, size_t len)
{
	const char* sid = get_sid();
	if (sid == NULL || *sid == 0)
		return false;

	command_->clear();
	if (command_->hset(sid, name, (const char*) value, len) < 0)
		return false;
	time_t ttl = get_ttl();
	if (ttl > 0)
		return set_timeout(ttl);
	return true;
}

const session_string* redis_session::get_buf(const char* name)
{
	command_->clear();
	const char* sid = get_sid();
	if (sid == NULL || *sid == 0)
		return NULL;

	// �ȳ��Դӻ�����л��һ�������������û�к��ʵģ��򴴽��µĻ���������
	// ����֮������������У��Ա��´��ظ���ѯ��ͬ����ʱʹ��

	session_string* ss;
	std::map<string, session_string*>::iterator it = buffers_.find(name);
	if (it == buffers_.end())
	{
		ss = NEW session_string;
		buffers_[name] = ss;
	}
	else
	{
		ss = it->second;
		ss->clear();
	}

	if (command_->hget(sid, name, *ss) == false)
		return NULL;
	return ss;
}

bool redis_session::del(const char* name)
{
	const char* sid = get_sid();
	if (sid == NULL || *sid == 0)
		return false;

	command_->clear();
	return command_->hdel(sid, name, NULL) >= 0 ? true : false;
}

bool redis_session::set_attrs(const std::map<string, session_string>& attrs)
{
	const char* sid = get_sid();
	if (sid == NULL || *sid == 0)
		return false;

	command_->clear();
	if (command_->hmset(sid, (const std::map<string, string>&) attrs) == false)
		return false;

	time_t ttl = get_ttl();
	if (ttl > 0)
		return set_timeout(ttl);
	else
		return true;
}

bool redis_session::get_attrs(std::map<string, session_string>& attrs)
{
	attrs_clear(attrs);

	const char* sid = get_sid();
	if (sid == NULL || *sid == 0)
		return false;

	command_->clear();
	if (command_->hgetall(sid, (std::map<string, string>&) attrs) == false)
		return false;
	return true;
}

bool redis_session::remove()
{
	const char* sid = get_sid();
	if (sid == NULL || *sid == 0)
		return false;

	command_->clear();
	return command_->del(sid, NULL) >= 0 ? true : false;
}

bool redis_session::set_timeout(time_t ttl)
{
	const char* sid = get_sid();
	if (sid == NULL || *sid == 0)
		return false;

	command_->clear();
	return command_->expire(sid, (int) ttl) > 0 ? true : false;
}

} // namespace acl