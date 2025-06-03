import json
import plotly.graph_objects as go

# 读取 JSON 文件
with open('../build/benchmark_results.json', 'r') as f:
    data = json.load(f)

benchmark_names = []
times = []
for run in data['benchmarks']:
    benchmark_names.append(run['name'])
    times.append(run['cpu_time'])

# 绘制柱状图
fig = go.Figure(data=[go.Bar(x=benchmark_names, y=times)])
fig.update_layout(
    title='Benchmark Results',
    xaxis_title='Benchmark',
    yaxis_title='CPU Time (ns)',
    xaxis_tickangle=-45
)
fig.show()