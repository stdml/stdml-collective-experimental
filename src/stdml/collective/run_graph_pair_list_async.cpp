#include <cassert>
#include <execution>
#include <iostream>

#include <stdml/bits/collective/execution.hpp>
#include <stdml/bits/collective/log.hpp>
#include <stdml/bits/collective/session.hpp>
#include <stdml/bits/collective/task.hpp>

namespace stdml::collective
{
class collective_task : public task
{
  protected:
    workspace_state *state;
    bool finished_;

  public:
    collective_task(workspace_state *state) : state(state), finished_(false)
    {
    }
};

class recv_onto : public collective_task
{
    mailbox::Q *q;

  public:
    recv_onto(const peer_id id, const session *sess, workspace_state *state)
        : collective_task(state), q(sess->mailbox_->require(id, (*state)->name))
    {
    }

    void poll() override
    {
        if (auto b = q->try_get(); b.has_value()) {
            state->add_to(b->data.get());
            finished_ = true;
        }
    }

    bool finished() override
    {
        return finished_;
    }
};

class recv_into : public collective_task
{
    slotbox::S *s;

  public:
    recv_into(const peer_id id, const session *sess, workspace_state *state)
        : collective_task(state), s(sess->slotbox_->require(id, (*state)->name))
    {
        s->waitQ.put((*state)->recv);
    }

    void poll() override
    {
        if (auto ptr = s->recvQ.try_get(); ptr.has_value()) {
            assert((*ptr) == (*state)->recv);
            finished_ = true;
        }
    }

    bool finished() override
    {
        return finished_;
    }
};

class send_task : public collective_task
{
    const peer_id id;
    rchan::client *client;
    bool reduce;

  public:
    send_task(const peer_id id, const session *sess, workspace_state *state,
              bool reduce)
        : collective_task(state),
          id(id),
          client(sess->client_pool_->require(rchan::conn_collective)),
          reduce(reduce)
    {
    }

    void poll() override
    {
        uint32_t flags = 0;
        if (!reduce) {
            flags |= rchan::message_header::wait_recv_buf;
        }
        client->send(id, (*state)->name.c_str(), state->effective_data(),
                     (*state)->data_size(), flags);
        finished_ = true;
    }

    bool finished() override
    {
        return finished_;
    }
};

struct recv {
    const session *sess;
    workspace_state *state;
    const bool reduce;

    recv(const session *sess, workspace_state *state, bool reduce)
        : sess(sess), state(state), reduce(reduce)
    {
    }

    task *operator()(const peer_id &id) const
    {
        if (reduce) {
            // return new recv_onto(id, sess, state);
            return new monitored_task(recv_onto(id, sess, state));
        } else {
            // return new recv_into(id, sess, state);
            return new monitored_task(recv_into(id, sess, state));
        }
    }
};

struct send {
    const session *sess;
    workspace_state *state;
    const bool reduce;

    send(const session *sess, workspace_state *w, bool reduce)
        : sess(sess), state(w), reduce(reduce)
    {
    }

    task *operator()(const peer_id &id) const
    {
        // return new send_task(id, sess, state, reduce);
        return new monitored_task(send_task(id, sess, state, reduce));
    }
};

task *run_graphs_async(session *sess, const workspace *w,
                       const std::vector<const graph *> &gs)
{
    auto rt = sess->runtime_.get();
    const auto &peers = sess->peers();
    auto rank = sess->rank();

    auto state = new workspace_state(w);
    task_builder steps;
    for (const auto g : gs) {
        const auto prevs = peers[g->prevs(rank)];
        const auto nexts = peers[g->nexts(rank)];
        if (g->self_loop(rank)) {
            steps << task::par(task::fmap(recv(sess, state, true), prevs), rt);
            steps << task::par(task::fmap(send(sess, state, true), nexts), rt);
        } else {
            if (prevs.size() == 0) {
                steps << new simple_task([=] {
                    if (state->recv_count() == 0) {
                        state->forward();
                    }
                });
            } else {
                steps << task::seq(task::fmap(recv(sess, state, false), prevs));
            }
            steps << task::par(task::fmap(send(sess, state, false), nexts), rt);
        }
    }
    steps << new simple_task([=] { delete state; });
    return steps.seq();
}

size_t run_graph_pair_list_async(session *sess, const workspace &w,
                                 const graph_pair_list &gps, size_t chunk_size)
{
    task_cnt = 0;
    const auto pw = split_work(w, gps, chunk_size);
    const auto f = [&](auto &wgp) {
        const auto &[w, gp] = wgp;
        return run_graphs_async(sess, &w, gp);
    };
    auto t = task::par(task::fmap(f, pw), sess->runtime_.get());
    if (pw.size() > 1) {
        log() << "splitted workspace into" << pw.size() << "parts";
    }
    log() << "created" << task_cnt << "task";
    t->finish();
    delete t;
    return pw.size();
}
}  // namespace stdml::collective
