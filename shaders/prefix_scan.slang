static const uint GROUP_SIZE = 64;

// RWStructuredBuffer<int> group_sums;

groupshared uint shared_data[GROUP_SIZE];

[shader("compute")]
[numthreads(GROUP_SIZE, 1, 1)]
void computeMain(
    RWStructuredBuffer<int> output,
    uint3 dispatchThreadID : SV_DispatchThreadID,
    uint3 groupThreadID    : SV_GroupThreadID,
    uint3 groupID          : SV_GroupID)
{
    uint global_idx = dispatchThreadID.x;
    uint tid = groupThreadID.x;
    uint group_id = groupID.x;

    shared_data[tid] = output[global_idx];
    GroupMemoryBarrierWithGroupSync();

    for (uint stride = 1; stride < GROUP_SIZE; stride <<= 1)
    {
        uint index = (tid + 1) * stride * 2 - 1;
        if (index < GROUP_SIZE)
            shared_data[index] += shared_data[index - stride];
        GroupMemoryBarrierWithGroupSync();
    }

    // if (tid == GROUP_SIZE - 1)
    //     group_sums[group_id.x] = shared_data[tid];

    if (tid == 0)
        shared_data[GROUP_SIZE - 1] = 0;
    GroupMemoryBarrierWithGroupSync();

    for (uint stride = GROUP_SIZE / 2; stride >= 1; stride >>= 1)
    {
        uint index = (tid + 1) * stride * 2 - 1;
        if (index < GROUP_SIZE)
        {
            uint temp = shared_data[index - stride];
            shared_data[index - stride] = shared_data[index];
            shared_data[index] += temp;
        }
        GroupMemoryBarrierWithGroupSync();

        if (stride == 1) break;
    }

    output[global_idx] = shared_data[tid];
}