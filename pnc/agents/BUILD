package(default_visibility = ["//visibility:public"])

cc_library(
    name = "agents",
    srcs = ["agents.cc"],
    hdrs = ["agents.h"],
    deps = [
        "//pnc/agents/sample:sample_agent",
		"//pnc/agents/hqztrue:hqztrue_agent",
		"//pnc/agents/table:table_agent",
        "//pnc/simulation:vehicle_agent_factory",
    ],
    alwayslink = 1,
)
