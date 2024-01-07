create function aws_s3_endpoint() returns s3_endpoint
    immutable
    language sql
as
$$
select row ('https://${bucket}.s3.${region}.amazonaws.com')::omni_aws.s3_endpoint
$$;