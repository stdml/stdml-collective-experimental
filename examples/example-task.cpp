#include <stdml/bits/collective/log.hpp>
#include <stdml/bits/collective/task.hpp>

stdml::collective::task *new_task(const std::string msg)
{
    return new stdml::collective::simple_task(
        [=] { stdml::collective::log() << msg; });
}

stdml::collective::task *new_seq_task(const std::string m1,
                                      const std::string m2)
{
    stdml::collective::task_builder b;
    b << new_task(m1);
    b << new_task(m2);
    return b.seq();
}

int main()
{
    stdml::collective::enabled_log();
    stdml::collective::task_builder b;
    b << new_seq_task("11", "12");
    b << new_seq_task("21", "22");
    auto t = b.par();
    t->finish();
    delete t;
    return 0;
}
