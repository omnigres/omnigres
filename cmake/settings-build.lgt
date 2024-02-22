:- initialization((
        set_logtalk_flag(prolog_loader, [silent(true)]),
        % be silent except for warnings (set to "off" for final deliverable)
        set_logtalk_flag(report, warnings),
        %set_logtalk_flag(report, off),
        % prevent reloading of embedded code
        set_logtalk_flag(reload, skip),
        % optimize performance
        set_logtalk_flag(optimize, on),
        % do not save source file data
        set_logtalk_flag(source_data, off),
        % lock your entities by default to prevent breaking encapsulation
        set_logtalk_flag(events, deny),
        set_logtalk_flag(complements, deny),
        set_logtalk_flag(dynamic_declarations, deny),
        set_logtalk_flag(context_switching_calls, deny)
)).