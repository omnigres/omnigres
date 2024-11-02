-- Namespaces
create view namespaces_with_data as
select (data -> 'metadata' ->> 'uid')::uuid       as uid,
       data -> 'metadata' ->> 'name'              as name,
       data -> 'metadata' ->> 'creationTimestamp' as creation_timestamp,
       data -> 'status' ->> 'phase'               as phase,
       data
from (select jsonb_array_elements((api('/api/v1/namespaces'))::jsonb -> 'items') as data)
         as namespaces;

create view namespaces as
select uid, name, creation_timestamp, phase
from namespaces_with_data;

-- Pods
create view pods_with_data as
select (data -> 'metadata' ->> 'uid')::uuid                                                                   as uid,
       data -> 'metadata' ->> 'name'                                                                          as name,
       data -> 'metadata' ->> 'namespace'                                                                     as namespace,
       data -> 'metadata' ->> 'creationTimestamp' as creation_timestamp,
       data -> 'status' ->> 'phase'                                                                           as phase,
       data -> 'spec' ->> 'nodeName'                                                                          as node,
       data
from (select jsonb_array_elements((api('/api/v1/pods'))::jsonb -> 'items') as data)
         as pods;

create view pods as
select uid, name, namespace, creation_timestamp, phase, node
from pods_with_data;

create view pod_labels as
select uid, name, key, value
from pods_with_data,
     lateral (select key, value from jsonb_each(data -> 'metadata' -> 'labels') as label(key, value));

create view pod_ip_addresses as
select uid, name, ip_address
from pods_with_data,
     lateral (select jsonb_array_elements_text(jsonb_path_query_array(data, '$.status.podIPs[*].ip'))::inet as ip_address) as ip_addresses(ip_address);

-- Nodes

create view nodes_with_data as
select (data -> 'metadata' ->> 'uid')::uuid          as uid,
       data -> 'metadata' ->> 'name'                 as name,
       data -> 'metadata' ->> 'creationTimestamp'    as creation_timestamp,
       (data -> 'spec' ->> 'unschedulable')::boolean as unschedulable,
       data
from (select jsonb_array_elements((api('/api/v1/nodes'))::jsonb -> 'items') as data)
         as codes;

create view nodes as
select uid, name, creation_timestamp, unschedulable
from nodes_with_data;

create view node_cidrs as
select uid, name, cidr
from nodes_with_data,
     lateral (select jsonb_array_elements_text(jsonb_path_query_array(data, '$.spec.podCIDRs[*]'))::cidr as cidr) as cidrs(cidr);

create view node_images as
select data -> 'metadata' ->> 'name'                            as node,
       jsonb_array_elements(case
                                when jsonb_typeof(image_data -> 'names') = 'null' then '[
                                  null
                                ]'::jsonb
                                else image_data -> 'names' end) as image,
       image_data ->> 'sizeBytes' as size_bytes
from (select jsonb_array_elements((api('/api/v1/nodes'))::jsonb -> 'items') as data)
         as codes,
     lateral (select jsonb_path_query(data, '$.status.images[*]') as image_data ) images;

create view node_labels as
select uid, name, key, value
from nodes_with_data,
     lateral (select key, value from jsonb_each(data -> 'metadata' -> 'labels') as label(key, value));

-- Services

create view services_with_data as
select (data -> 'metadata' ->> 'uid')::uuid       as uid,
       data -> 'metadata' ->> 'name'              as name,
       data -> 'metadata' ->> 'namespace'         as namespace,
       data -> 'metadata' ->> 'creationTimestamp' as creation_timestamp,
       data -> 'spec' ->> 'type'                  as type,
       data -> 'spec' ->> 'externalName'          as external_name,
       data -> 'spec' ->> 'internalTrafficPolicy' as internal_traffic_policy,
       data -> 'spec' ->> 'externalTrafficPolicy' as external_traffic_policy,
       data -> 'spec' ->> 'ipFamilyPolicy'        as ip_family_policy,
       data
from (select jsonb_array_elements((api('/api/v1/services'))::jsonb -> 'items') as data)
         as services;

create view services as
select uid,
       name,
       namespace,
       creation_timestamp,
       type,
       external_name,
       internal_traffic_policy,
       external_traffic_policy,
       ip_family_policy
from services_with_data;

create view service_ports as
select uid,
       name,
       port ->> 'name'              as port_name,
       (port ->> 'port')::int       as port,
       (port ->> 'targetPort')::int as target_port,
       port ->> 'protocol'          as protocol
from services_with_data,
     lateral (select jsonb_path_query(data, '$.spec.ports[*]') as port) as ports(port);

create view service_cluster_ips as
select uid,
       name,
       ip_address
from services_with_data,
     lateral (select (jsonb_path_query(data, '$.spec.clusterIPs[*]') #>> '{}')::inet as ip_address) as ips(ip_address);

create view service_external_ips as
select uid,
       name,
       ip_address
from services_with_data,
     lateral (select (jsonb_path_query(data, '$.spec.externalIPs[*]') #>> '{}')::inet as ip_address) as ips(ip_address);

create view service_ip_families as
select uid,
       name,
       ip_family
from services_with_data,
     lateral (select (jsonb_path_query(data, '$.spec.ipFamilies[*]') #>> '{}') as ip_family) as ip_families(ip_family);

create view service_labels as
select uid, name, key, value
from services_with_data,
     lateral (select key, value from jsonb_each(data -> 'metadata' -> 'labels') as label(key, value));

--- Jobs

create view jobs_with_data as
select (data -> 'metadata' ->> 'uid')::uuid       as uid,
       data -> 'metadata' ->> 'name'              as name,
       data -> 'metadata' ->> 'creationTimestamp' as creation_timestamp,
       data -> 'status' ->> 'startTime'           as start_time,
       data -> 'status' ->> 'completionTime'      as completion_time,
       data -> 'spec' ->> 'completionMode'        as completion_mode,
       (data -> 'spec' ->> 'completions')::int    as completions,
       (data -> 'spec' ->> 'parallelism')::int    as parallelism,
       (data -> 'status' ->> 'active')::int       as active_pods,
       (data -> 'status' ->> 'failed')::int       as failed_pods,
       (data -> 'status' ->> 'ready')::int        as ready_pods,
       (data -> 'status' ->> 'succeeded')::int    as succeeded_pods,
       data
from (select jsonb_array_elements((api('/apis/batch/v1/jobs'))::jsonb -> 'items') as data)
         as jobs;

create view jobs as
select uid,
       name,
       creation_timestamp,
       start_time,
       completion_time,
       completion_mode,
       completions,
       parallelism,
       active_pods,
       failed_pods,
       ready_pods,
       succeeded_pods
from jobs_with_data;

create view job_labels as
select uid, name, key, value
from jobs_with_data,
     lateral (select key, value from jsonb_each(data -> 'metadata' -> 'labels') as label(key, value));
